/*
 * siclint.c — instrumentum mandati lineae pro XML
 *
 * Simile xmllint, sed SIC bibliotheca utitur.
 *
 * Usus:
 *   siclint plica.xml                  — verifica formam rectam
 *   siclint --format plica.xml         — scribe formatam
 *   siclint --noblanks plica.xml       — sine spatiis vacuis
 *   siclint --xpath EXPR plica.xml     — quaere per viam
 *   siclint --shell plica.xml          — modus interactivus
 *   siclint --output plica.xml in.xml  — scribe in plicam
 *   siclint --noout plica.xml          — nihil scribe (solum verifica)
 *   siclint -                          — lege ex stdin
 */
#include "sic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ================================================================
 * auxiliaria
 * ================================================================ */

static void morire(const char *nuntius)
{
    fprintf(stderr, "siclint: %s\n", nuntius);
    exit(1);
}

static void usus(void)
{
    fputs(
        "Usus: siclint [optiones] plica.xml\n"
        "\n"
        "Optiones:\n"
        "  --format       scribe XML formatam (indentationem adde)\n"
        "  --noblanks     praetermitte nodos textus vacuos\n"
        "  --noout        nihil scribe (solum verifica formam)\n"
        "  --output VIA   scribe in plicam (non stdout)\n"
        "  --xpath EXPR   quaere elementa per viam simplicem\n"
        "  --shell        modus interactivus\n"
        "  --encode COD   codificatio exitus (praefinitum: UTF-8)\n"
        "  --version      monstra versionem\n"
        "  -              lege ex stdin\n",
        stderr
    );
    exit(1);
}

/* ================================================================
 * stdin in plicam temporalem
 * ================================================================ */

static char *lege_stdin(void)
{
    size_t cap   = 4096, lon = 0;
    char *alveus = (char *)malloc(cap);
    if (!alveus)
        morire("memoria exhausta");

    size_t n;
    while ((n = fread(alveus + lon, 1, cap - lon, stdin)) > 0) {
        lon += n;
        if (lon == cap) {
            cap *= 2;
            alveus = (char *)realloc(alveus, cap);
            if (!alveus)
                morire("memoria exhausta");
        }
    }

    /* scribe in plicam temporalem */
    char *via = strdup("/tmp/siclint_stdin_XXXXXX");
    int fd    = mkstemp(via);
    if (fd < 0) {
        free(alveus);
        free(via);
        morire("mkstemp falsum");
    }
    FILE *fp = fdopen(fd, "w");
    fwrite(alveus, 1, lon, fp);
    fclose(fp);
    free(alveus);
    return via;
}

/* ================================================================
 * scriptor nodorum singulorum (pro xpath et shell)
 * ================================================================ */

static void w_escaped(FILE *fp, const sic_char_t *s, int attr)
{
    if (!s)
        return;
    for (; *s; s++) {
        switch (*s) {
        case '&': fputs("&amp;", fp);  break;
        case '<': fputs("&lt;", fp);   break;
        case '>': if (!attr)
                fputs("&gt;", fp);
            else
                fputc(*s, fp);
            break;
        case '"': if (attr)
                fputs("&quot;", fp);
            else
                fputc(*s, fp);
            break;
        default:  fputc(*s, fp);       break;
        }
    }
}

static void w_nodus(
    FILE *fp, sic_node_ptr_t nodus, int altitudo,
    int forma
) {
    if (!nodus)
        return;

    if (
        nodus->type == SIC_TEXT_NODE ||
        nodus->type == SIC_CDATA_SECTION_NODE
    ) {
        w_escaped(fp, nodus->content, 0);
        return;
    }
    if (nodus->type != SIC_ELEMENT_NODE)
        return;

    if (forma && altitudo > 0) {
        fputc('\n', fp);
        for (int i = 0; i < altitudo; i++)
            fputs("  ", fp);
    }

    fputc('<', fp);
    if (nodus->ns && nodus->ns->prefix)
        fprintf(fp, "%s:", nodus->ns->prefix);
    fputs((const char *)nodus->name, fp);

    for (sic_ns_ptr_t ns = nodus->nsDef; ns; ns = ns->next) {
        if (ns->prefix)
            fprintf(fp, " xmlns:%s=\"", ns->prefix);
        else
            fputs(" xmlns=\"", fp);
        w_escaped(fp, ns->href, 1);
        fputc('"', fp);
    }

    for (sic_attr_ptr_t a = nodus->properties; a; a = a->next) {
        fputc(' ', fp);
        if (a->ns && a->ns->prefix)
            fprintf(fp, "%s:", a->ns->prefix);
        fprintf(fp, "%s=\"", a->name);
        w_escaped(fp, a->value, 1);
        fputc('"', fp);
    }

    if (!nodus->children) {
        fputs("/>", fp);
        return;
    }

    fputc('>', fp);

    int habet_elem = 0;
    for (sic_node_ptr_t c = nodus->children; c; c = c->next)
        if (c->type == SIC_ELEMENT_NODE) {
        habet_elem = 1;
        break;
    }

    for (sic_node_ptr_t c = nodus->children; c; c = c->next)
        w_nodus(fp, c, altitudo + 1, forma && habet_elem);

    if (forma && habet_elem) {
        fputc('\n', fp);
        for (int i = 0; i < altitudo; i++)
            fputs("  ", fp);
    }

    fputs("</", fp);
    if (nodus->ns && nodus->ns->prefix)
        fprintf(fp, "%s:", nodus->ns->prefix);
    fprintf(fp, "%s>", nodus->name);
}

/* ================================================================
 * xpath simplex — /a/b/c vel //nomen
 * ================================================================ */

/* collige omnes descendentes cum nomine */
static void xpath_collect_desc(
    sic_node_ptr_t nodus,
    const char *nomen,
    sic_node_ptr_t **res, int *n, int *cap
) {
    for (sic_node_ptr_t c = nodus->children; c; c = c->next) {
        if (c->type != SIC_ELEMENT_NODE)
            continue;
        if (strcmp((const char *)c->name, nomen) == 0) {
            if (*n >= *cap) {
                *cap = *cap ? *cap * 2 : 32;
                *res = (sic_node_ptr_t *)realloc(
                    *res, (size_t)*cap * sizeof(sic_node_ptr_t)
                );
            }
            (*res)[(*n)++] = c;
        }
        xpath_collect_desc(c, nomen, res, n, cap);
    }
}

/* collige filios directos cum nomine */
static void xpath_collect_child(
    sic_node_ptr_t nodus,
    const char *nomen,
    sic_node_ptr_t **res, int *n, int *cap
) {
    for (sic_node_ptr_t c = nodus->children; c; c = c->next) {
        if (c->type != SIC_ELEMENT_NODE)
            continue;
        /* '*' congruit cum omnibus */
        if (
            strcmp(nomen, "*") == 0 ||
            strcmp((const char *)c->name, nomen) == 0
        ) {
            if (*n >= *cap) {
                *cap = *cap ? *cap * 2 : 32;
                *res = (sic_node_ptr_t *)realloc(
                    *res, (size_t)*cap * sizeof(sic_node_ptr_t)
                );
            }
            (*res)[(*n)++] = c;
        }
    }
}

/*
 * disseca expressionem xpath simplicem:
 *   /a/b/c    — via absoluta
 *   //nomen   — omnes descendentes
 *   a/b       — via relativa a radice
 *   @attr     — valor attributi
 *
 * reddit indicem nodorum inventorum.
 */
static void xpath_eval(
    sic_doc_ptr_t doc, const char *expr,
    sic_node_ptr_t **res, int *n_res,
    char **attr_nomen
) {
    *res        = NULL;
    *n_res      = 0;
    *attr_nomen = NULL;
    int cap     = 0;

    sic_node_ptr_t radix = sic_doc_get_root_element(doc);
    if (!radix)
        return;

    const char *p = expr;

    /* //nomen — quaere per omnes descendentes */
    if (p[0] == '/' && p[1] == '/') {
        p += 2;
        /* praeterire spatia nominum (si praefixum:nomen, cape nomen) */
        const char *colon = strchr(p, ':');
        const char *nomen = colon ? colon + 1 : p;
        /* si finitur @attr */
        const char *at = strchr(nomen, '/');
        char nomen_buf[256];
        if (at && at[1] == '@') {
            snprintf(
                nomen_buf, sizeof(nomen_buf), "%.*s",
                (int)(at - nomen), nomen
            );
            *attr_nomen = strdup(at + 2);
            nomen       = nomen_buf;
        }
        xpath_collect_desc(radix, nomen, res, n_res, &cap);
        /* radix ipsa quoque */
        if (strcmp((const char *)radix->name, nomen) == 0) {
            if (*n_res >= cap) {
                cap = cap ? cap * 2 : 32;
                *res = (sic_node_ptr_t *)realloc(
                    *res, (size_t)cap * sizeof(sic_node_ptr_t)
                );
            }
            (*res)[(*n_res)++] = radix;
        }
        return;
    }

    /* via absoluta vel relativa */
    if (*p == '/')
        p++;

    /* gradi per segmenta */
    sic_node_ptr_t *currentes = NULL;
    int n_curr = 0;
    int cap_curr = 0;

    /* primus gradus: congrue cum radice */
    char seg[256];
    const char *slash = strchr(p, '/');
    if (slash) {
        snprintf(seg, sizeof(seg), "%.*s", (int)(slash - p), p);
        p = slash + 1;
    } else {
        snprintf(seg, sizeof(seg), "%s", p);
        p = p + strlen(p);
    }

    /* praeterire praefixum spatii nominum */
    const char *colon = strchr(seg, ':');
    const char *loc   = colon ? colon + 1 : seg;

    if (
        strcmp(loc, "*") == 0 ||
        strcmp((const char *)radix->name, loc) == 0
    ) {
        cap_curr = 8;
        currentes = (sic_node_ptr_t *)malloc(
            (size_t)cap_curr * sizeof(sic_node_ptr_t)
        );
        currentes[0] = radix;
        n_curr       = 1;
    }

    /* ceteri gradus */
    while (*p && n_curr > 0) {
        /* si @attr */
        if (*p == '@') {
            *attr_nomen = strdup(p + 1);
            break;
        }

        slash = strchr(p, '/');
        if (slash) {
            snprintf(seg, sizeof(seg), "%.*s", (int)(slash - p), p);
            p = slash + 1;
        } else {
            snprintf(seg, sizeof(seg), "%s", p);
            p = p + strlen(p);
        }

        /* si @attr in fine segmenti */
        char *at = strchr(seg, '[');
        if (at)
            *at = '\0'; /* praetermitte praedicata */

        colon = strchr(seg, ':');
        loc   = colon ? colon + 1 : seg;

        /* si ultimum segmentum est @attr */
        if (loc[0] == '@') {
            *attr_nomen = strdup(loc + 1);
            break;
        }

        sic_node_ptr_t *proximi = NULL;
        int n_prox = 0, cap_prox = 0;

        for (int i = 0; i < n_curr; i++)
            xpath_collect_child(
                currentes[i], loc,
                &proximi, &n_prox, &cap_prox
            );

        free(currentes);
        currentes = proximi;
        n_curr    = n_prox;
        cap_curr  = cap_prox;
    }

    *res   = currentes;
    *n_res = n_curr;
}

static void xpath_print_results(
    FILE *fp, sic_node_ptr_t *nodi,
    int n, const char *attr_nomen,
    int forma
) {
    for (int i = 0; i < n; i++) {
        if (attr_nomen) {
            sic_char_t *v = sic_get_prop(
                nodi[i],
                (const sic_char_t *)attr_nomen
            );
            if (v) {
                fprintf(fp, "%s\n", (const char *)v);
                sic_free(v);
            }
        } else {
            w_nodus(fp, nodi[i], 0, forma);
            fputc('\n', fp);
        }
    }
}

/* ================================================================
 * census — numeratio elementorum
 * ================================================================ */

static void census_r(
    sic_node_ptr_t nodus, int *elementa, int *textus,
    int *attributa
) {
    for (sic_node_ptr_t c = nodus->children; c; c = c->next) {
        if (c->type == SIC_ELEMENT_NODE) {
            (*elementa)++;
            for (sic_attr_ptr_t a = c->properties; a; a = a->next)
                (*attributa)++;
            census_r(c, elementa, textus, attributa);
        } else if (c->type == SIC_TEXT_NODE) {
            (*textus)++;
        }
    }
}

static void census(sic_doc_ptr_t doc)
{
    sic_node_ptr_t radix = sic_doc_get_root_element(doc);
    if (!radix) {
        printf("documentum vacuum\n");
        return;
    }

    int elementa = 1, textus = 0, attributa = 0;
    for (sic_attr_ptr_t a = radix->properties; a; a = a->next)
        attributa++;
    census_r(radix, &elementa, &textus, &attributa);
    printf("elementa:  %d\n", elementa);
    printf("textus:    %d\n", textus);
    printf("attributa: %d\n", attributa);
}

/* ================================================================
 * modus interactivus (shell)
 * ================================================================ */

static void shell_auxilium(void)
{
    printf(
        "Mandata:\n"
        "  ls [via]        — enumera filios\n"
        "  cat [via]       — monstra contentum textus\n"
        "  attr [via]      — monstra attributa\n"
        "  xpath EXPR      — quaere\n"
        "  census          — numeratio\n"
        "  scribe VIA      — salva in plicam\n"
        "  auxilium        — hoc nuntium\n"
        "  exi             — exi\n"
    );
}

/* resolve via simplex: /a/b/c a radice */
static sic_node_ptr_t shell_resolve(
    sic_node_ptr_t radix,
    const char *via
) {
    if (!via || !*via || strcmp(via, "/") == 0)
        return radix;
    const char *p = via;
    if (*p == '/')
        p++;

    sic_node_ptr_t curr = radix;
    char seg[256];
    while (*p && curr) {
        const char *slash = strchr(p, '/');
        if (slash) {
            snprintf(seg, sizeof(seg), "%.*s", (int)(slash - p), p);
            p = slash + 1;
        } else {
            snprintf(seg, sizeof(seg), "%s", p);
            p = p + strlen(p);
        }
        if (!*seg)
            continue;

        /* praeterire praefixum */
        const char *colon = strchr(seg, ':');
        const char *loc   = colon ? colon + 1 : seg;

        sic_node_ptr_t found = NULL;
        for (sic_node_ptr_t c = curr->children; c; c = c->next) {
            if (
                c->type == SIC_ELEMENT_NODE &&
                strcmp((const char *)c->name, loc) == 0
            ) {
                found = c;
                break;
            }
        }
        curr = found;
    }
    return curr;
}

static void shell(sic_doc_ptr_t doc)
{
    sic_node_ptr_t radix = sic_doc_get_root_element(doc);
    if (!radix) {
        printf("documentum vacuum\n");
        return;
    }

    printf("siclint shell — scribe 'auxilium' pro mandatis\n");
    printf("radix: <%s>\n", (const char *)radix->name);

    char linea[1024];
    while (1) {
        printf("> ");
        fflush(stdout);
        if (!fgets(linea, sizeof(linea), stdin))
            break;

        /* tolle '\n' */
        size_t lon = strlen(linea);
        if (lon > 0 && linea[lon - 1] == '\n')
            linea[lon - 1] = '\0';

        if (linea[0] == '\0')
            continue;

        if (strcmp(linea, "exi") == 0 || strcmp(linea, "q") == 0)
            break;

        if (strcmp(linea, "auxilium") == 0) {
            shell_auxilium();
            continue;
        }

        if (strcmp(linea, "census") == 0) {
            census(doc);
            continue;
        }

        if (strncmp(linea, "scribe ", 7) == 0) {
            const char *via = linea + 7;
            while (*via == ' ')
                via++;
            int r = sic_save_format_file_enc(via, doc, "UTF-8", 1);
            if (r == 0)
                printf("scriptum: %s\n", via);
            else
                printf("error scribendi\n");
            continue;
        }

        if (strncmp(linea, "xpath ", 6) == 0) {
            const char *expr = linea + 6;
            while (*expr == ' ')
                expr++;
            sic_node_ptr_t *nodi;
            int n;
            char *attr_n;
            xpath_eval(doc, expr, &nodi, &n, &attr_n);
            if (n == 0) {
                printf("(nulli nodi inventi)\n");
            } else {
                printf("(%d nodi inventi)\n", n);
                xpath_print_results(stdout, nodi, n, attr_n, 1);
            }
            free(nodi);
            free(attr_n);
            continue;
        }

        /* ls [via] */
        if (
            strncmp(linea, "ls", 2) == 0 &&
            (linea[2] == '\0' || linea[2] == ' ')
        ) {
            const char *via = linea[2] ? linea + 3 : NULL;
            while (via && *via == ' ')
                via++;
            sic_node_ptr_t nodus = shell_resolve(radix, via);
            if (!nodus) {
                printf("non inventum\n");
                continue;
            }
            for (sic_node_ptr_t c = nodus->children; c; c = c->next) {
                if (c->type == SIC_ELEMENT_NODE) {
                    int n_attr = 0;
                    for (
                        sic_attr_ptr_t a = c->properties;
                        a;
                        a = a->next
                    ) n_attr++;
                    int n_ch = 0;
                    for (
                        sic_node_ptr_t gc = c->children;
                        gc;
                        gc = gc->next
                    )
                        if (gc->type == SIC_ELEMENT_NODE)
                            n_ch++;
                    printf("  <%s>", (const char *)c->name);
                    if (n_attr)
                        printf("  [%d attr]", n_attr);
                    if (n_ch)
                        printf("  (%d filii)", n_ch);
                    printf("\n");
                } else if (c->type == SIC_TEXT_NODE && c->content) {
                    size_t cl = strlen((const char *)c->content);
                    if (cl > 60)
                        printf(
                            "  \"%.57s...\"\n",
                            (const char *)c->content
                        );
                    else
                        printf(
                            "  \"%s\"\n",
                            (const char *)c->content
                        );
                }
            }
            continue;
        }

        /* cat [via] */
        if (
            strncmp(linea, "cat", 3) == 0 &&
            (linea[3] == '\0' || linea[3] == ' ')
        ) {
            const char *via = linea[3] ? linea + 4 : NULL;
            while (via && *via == ' ')
                via++;
            sic_node_ptr_t nodus = shell_resolve(radix, via);
            if (!nodus) {
                printf("non inventum\n");
                continue;
            }
            sic_char_t *cont = sic_node_get_content(nodus);
            if (cont) {
                printf("%s\n", (const char *)cont);
                sic_free(cont);
            } else {
                printf("(vacuum)\n");
            }
            continue;
        }

        /* attr [via] */
        if (
            strncmp(linea, "attr", 4) == 0 &&
            (linea[4] == '\0' || linea[4] == ' ')
        ) {
            const char *via = linea[4] ? linea + 5 : NULL;
            while (via && *via == ' ')
                via++;
            sic_node_ptr_t nodus = shell_resolve(radix, via);
            if (!nodus) {
                printf("non inventum\n");
                continue;
            }
            if (!nodus->properties) {
                printf("(nulla attributa)\n");
                continue;
            }
            for (sic_attr_ptr_t a = nodus->properties; a; a = a->next) {
                if (a->ns && a->ns->prefix)
                    printf(
                        "  %s:%s = \"%s\"\n",
                        (const char *)a->ns->prefix,
                        (const char *)a->name,
                        a->value ? (const char *)a->value : ""
                    );
                else
                    printf(
                        "  %s = \"%s\"\n",
                        (const char *)a->name,
                        a->value ? (const char *)a->value : ""
                    );
            }
            continue;
        }

        printf("mandatum ignotum: %s\n", linea);
    }
}

/* ================================================================
 * principalis
 * ================================================================ */

int main(int argc, char **argv)
{
    int forma       = 0;
    int noblanks    = 0;
    int noout       = 0;
    int do_census   = 0;
    int do_shell    = 0;
    const char *via_exitus   = NULL;
    const char *codificatio  = "UTF-8";
    const char *xpath_expr   = NULL;
    const char *via_plicae   = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--format") == 0) {
            forma = 1;
        } else if (strcmp(argv[i], "--noblanks") == 0) {
            noblanks = 1;
        } else if (strcmp(argv[i], "--noout") == 0) {
            noout = 1;
        } else if (strcmp(argv[i], "--output") == 0) {
            if (++i >= argc)
                morire("--output requirit argumentum");
            via_exitus = argv[i];
        } else if (strcmp(argv[i], "--xpath") == 0) {
            if (++i >= argc)
                morire("--xpath requirit argumentum");
            xpath_expr = argv[i];
        } else if (strcmp(argv[i], "--encode") == 0) {
            if (++i >= argc)
                morire("--encode requirit argumentum");
            codificatio = argv[i];
        } else if (strcmp(argv[i], "--census") == 0) {
            do_census = 1;
        } else if (strcmp(argv[i], "--shell") == 0) {
            do_shell = 1;
        } else if (strcmp(argv[i], "--version") == 0) {
            printf("siclint (SIC) 0.1.0\n");
            return 0;
        } else if (
            strcmp(argv[i], "--help") == 0 ||
            strcmp(argv[i], "-h") == 0
        ) {
            usus();
        } else if (
            argv[i][0] == '-' && argv[i][1] != '\0' &&
            strcmp(argv[i], "-") != 0
        ) {
            fprintf(stderr, "siclint: optio ignota: %s\n", argv[i]);
            usus();
        } else {
            via_plicae = argv[i];
        }
    }

    if (!via_plicae)
        usus();

    /* lege ex stdin si '-' */
    char *via_temp = NULL;
    if (strcmp(via_plicae, "-") == 0) {
        via_temp   = lege_stdin();
        via_plicae = via_temp;
    }

    /* --format implicat --noblanks */
    if (forma)
        noblanks = 1;

    /* disseca */
    int vexilla = SIC_PARSE_NONET;
    if (noblanks)
        vexilla |= SIC_PARSE_NOBLANKS;

    sic_doc_ptr_t doc = sic_read_file(via_plicae, NULL, vexilla);
    if (via_temp) {
        remove(via_temp);
        free(via_temp);
    }

    if (!doc)
        morire("non potuit plicam dissecari");

    sic_node_ptr_t radix = sic_doc_get_root_element(doc);
    if (!radix)
        morire("documentum sine radice");

    /* census */
    if (do_census) {
        census(doc);
        sic_free_doc(doc);
        return 0;
    }

    /* modus interactivus */
    if (do_shell) {
        shell(doc);
        sic_free_doc(doc);
        return 0;
    }

    /* xpath */
    if (xpath_expr) {
        sic_node_ptr_t *nodi;
        int n;
        char *attr_n;
        xpath_eval(doc, xpath_expr, &nodi, &n, &attr_n);
        xpath_print_results(stdout, nodi, n, attr_n, forma);
        free(nodi);
        free(attr_n);
        sic_free_doc(doc);
        return (n > 0) ? 0 : 1;
    }

    /* scribe */
    if (!noout) {
        if (via_exitus) {
            int r = sic_save_format_file_enc(
                via_exitus, doc,
                codificatio, forma
            );
            if (r != 0)
                morire("error scribendi");
        } else {
            sic_save_format_fp(stdout, doc, codificatio, forma);
        }
    }

    sic_free_doc(doc);
    return 0;
}
