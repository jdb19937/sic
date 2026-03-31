/*
 * sic.c — Sermo Inscriptionis Capax — implementatio
 *
 * Dissector XML, arbor DOM, scriptor.
 * Sine ullis dependentiis externis.
 */
#include "sic.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================
 * alveus crescens — auxiliarium internum
 * ================================================================ */

typedef struct {
    char   *data;
    size_t  lon;
    size_t  cap;
} alveus_t;

static void alv_init(alveus_t *a)
{
    a->cap  = 64;
    a->lon  = 0;
    a->data = (char *)malloc(a->cap);
}

static void alv_push(alveus_t *a, char c)
{
    if (a->lon + 1 >= a->cap) {
        a->cap *= 2;
        a->data = (char *)realloc(a->data, a->cap);
    }
    a->data[a->lon++] = c;
}

static void alv_cat(alveus_t *a, const char *s, size_t n)
{
    while (a->lon + n >= a->cap) a->cap *= 2;
    a->data = (char *)realloc(a->data, a->cap);
    memcpy(a->data + a->lon, s, n);
    a->lon += n;
}

static sic_char_t *alv_fini(alveus_t *a)
{
    alv_push(a, '\0');
    return (sic_char_t *)a->data;
}

/* ================================================================
 * chorda auxiliaris
 * ================================================================ */

static sic_char_t *dup_str(const sic_char_t *s)
{
    if (!s) return NULL;
    size_t lon = strlen((const char *)s) + 1;
    sic_char_t *d = (sic_char_t *)malloc(lon);
    if (d) memcpy(d, s, lon);
    return d;
}

/* ================================================================
 * arbor — vincula et liberatio
 * ================================================================ */

/* propaga doc per omnes descendentes */
static void set_doc_r(sic_node_ptr_t nodus, sic_doc_ptr_t doc)
{
    nodus->doc = doc;
    for (sic_attr_ptr_t a = nodus->properties; a; a = a->next)
        a->doc = doc;
    for (sic_node_ptr_t c = nodus->children; c; c = c->next)
        set_doc_r(c, doc);
}

/* adde filium ad parentem */
static void link_child(sic_node_ptr_t parens, sic_node_ptr_t filius)
{
    filius->parent = parens;
    if (parens->doc) filius->doc = parens->doc;
    filius->next = NULL;
    filius->prev = parens->last;
    if (parens->last)
        parens->last->next = filius;
    else
        parens->children = filius;
    parens->last = filius;
}

static void free_ns_list(sic_ns_ptr_t ns)
{
    while (ns) {
        sic_ns_ptr_t prox = ns->next;
        free((void *)ns->href);
        free((void *)ns->prefix);
        free(ns);
        ns = prox;
    }
}

static void free_attr_list(sic_attr_ptr_t a)
{
    while (a) {
        sic_attr_ptr_t prox = a->next;
        free((void *)a->name);
        free(a->value);
        free(a);
        a = prox;
    }
}

static void free_node_r(sic_node_ptr_t n)
{
    if (!n) return;
    sic_node_ptr_t c = n->children;
    while (c) {
        sic_node_ptr_t prox = c->next;
        free_node_r(c);
        c = prox;
    }
    free_ns_list(n->nsDef);
    free_attr_list(n->properties);
    free((void *)n->name);
    free(n->content);
    free(n);
}

/* adde attributum ad finem catinae */
static void attr_append(sic_node_ptr_t nodus, sic_attr_ptr_t a)
{
    a->parent = nodus;
    a->doc    = nodus->doc;
    a->next   = NULL;
    if (!nodus->properties) {
        a->prev = NULL;
        nodus->properties = a;
    } else {
        sic_attr_ptr_t ult = nodus->properties;
        while (ult->next) ult = ult->next;
        ult->next = a;
        a->prev   = ult;
    }
}

/* ================================================================
 * documenta
 * ================================================================ */

sic_doc_ptr_t sic_new_doc(const sic_char_t *versio)
{
    sic_doc_ptr_t doc = (sic_doc_ptr_t)calloc(1, sizeof(sic_doc_t));
    doc->type    = SIC_DOCUMENT_NODE;
    doc->version = versio
        ? dup_str(versio)
        : dup_str((const sic_char_t *)"1.0");
    return doc;
}

void sic_free_doc(sic_doc_ptr_t doc)
{
    if (!doc) return;
    sic_node_ptr_t c = doc->children;
    while (c) {
        sic_node_ptr_t prox = c->next;
        free_node_r(c);
        c = prox;
    }
    free(doc->version);
    free(doc->encoding);
    free(doc);
}

sic_node_ptr_t sic_doc_get_root_element(sic_doc_ptr_t doc)
{
    if (!doc) return NULL;
    for (sic_node_ptr_t c = doc->children; c; c = c->next)
        if (c->type == SIC_ELEMENT_NODE) return c;
    return NULL;
}

sic_node_ptr_t sic_doc_set_root_element(sic_doc_ptr_t doc,
                                        sic_node_ptr_t radix)
{
    if (!doc || !radix) return NULL;

    /* tolle radicem veterem */
    sic_node_ptr_t vetus = sic_doc_get_root_element(doc);
    if (vetus) {
        if (vetus->prev) vetus->prev->next = vetus->next;
        else doc->children = vetus->next;
        if (vetus->next) vetus->next->prev = vetus->prev;
        else doc->last = vetus->prev;
        vetus->parent = NULL;
        vetus->prev   = NULL;
        vetus->next   = NULL;
    }

    /* insere novam radicem */
    radix->parent = NULL;
    radix->prev   = doc->last;
    radix->next   = NULL;
    if (doc->last)
        doc->last->next = radix;
    else
        doc->children = radix;
    doc->last = radix;

    set_doc_r(radix, doc);
    return vetus;
}

/* ================================================================
 * nodi — creatio
 * ================================================================ */

sic_node_ptr_t sic_new_node(sic_ns_ptr_t ns, const sic_char_t *nomen)
{
    sic_node_ptr_t n = (sic_node_ptr_t)calloc(1, sizeof(sic_node_t));
    n->type = SIC_ELEMENT_NODE;
    n->name = nomen ? dup_str(nomen) : NULL;
    n->ns   = ns;
    return n;
}

sic_node_ptr_t sic_new_text(const sic_char_t *contentum)
{
    sic_node_ptr_t n = (sic_node_ptr_t)calloc(1, sizeof(sic_node_t));
    n->type    = SIC_TEXT_NODE;
    n->content = contentum ? dup_str(contentum) : NULL;
    return n;
}

sic_node_ptr_t sic_new_child(sic_node_ptr_t parens, sic_ns_ptr_t ns,
                             const sic_char_t *nomen,
                             const sic_char_t *contentum)
{
    sic_node_ptr_t n = sic_new_node(ns, nomen);
    if (parens) link_child(parens, n);
    if (contentum) {
        sic_node_ptr_t t = sic_new_text(contentum);
        link_child(n, t);
    }
    return n;
}

sic_node_ptr_t sic_new_text_child(sic_node_ptr_t parens, sic_ns_ptr_t ns,
                                  const sic_char_t *nomen,
                                  const sic_char_t *contentum)
{
    return sic_new_child(parens, ns, nomen, contentum);
}

/* ================================================================
 * nodi — manipulatio
 * ================================================================ */

sic_node_ptr_t sic_add_child(sic_node_ptr_t parens, sic_node_ptr_t filius)
{
    if (!parens || !filius) return NULL;
    if (filius->parent) sic_unlink_node(filius);
    link_child(parens, filius);
    return filius;
}

sic_node_ptr_t sic_add_prev_sibling(sic_node_ptr_t frater,
                                    sic_node_ptr_t novus)
{
    if (!frater || !novus) return NULL;
    if (novus->parent) sic_unlink_node(novus);

    sic_node_ptr_t parens = frater->parent;
    novus->parent = parens;
    novus->doc    = frater->doc;
    novus->next   = frater;
    novus->prev   = frater->prev;

    if (frater->prev)
        frater->prev->next = novus;
    else if (parens)
        parens->children = novus;
    frater->prev = novus;

    return novus;
}

void sic_unlink_node(sic_node_ptr_t nodus)
{
    if (!nodus) return;
    if (nodus->prev)
        nodus->prev->next = nodus->next;
    else if (nodus->parent)
        nodus->parent->children = nodus->next;

    if (nodus->next)
        nodus->next->prev = nodus->prev;
    else if (nodus->parent)
        nodus->parent->last = nodus->prev;

    nodus->parent = NULL;
    nodus->prev   = NULL;
    nodus->next   = NULL;
}

void sic_free_node(sic_node_ptr_t nodus)
{
    free_node_r(nodus);
}

/* ================================================================
 * proprietates
 * ================================================================ */

sic_char_t *sic_get_prop(sic_node_ptr_t nodus, const sic_char_t *nomen)
{
    if (!nodus || !nomen) return NULL;
    for (sic_attr_ptr_t a = nodus->properties; a; a = a->next)
        if (!a->ns &&
            strcmp((const char *)a->name, (const char *)nomen) == 0)
            return a->value ? dup_str(a->value) : NULL;
    return NULL;
}

sic_char_t *sic_get_ns_prop(sic_node_ptr_t nodus,
                            const sic_char_t *nomen,
                            const sic_char_t *href)
{
    if (!nodus || !nomen) return NULL;
    for (sic_attr_ptr_t a = nodus->properties; a; a = a->next)
        if (strcmp((const char *)a->name, (const char *)nomen) == 0 &&
            a->ns && a->ns->href &&
            strcmp((const char *)a->ns->href, (const char *)href) == 0)
            return a->value ? dup_str(a->value) : NULL;
    return NULL;
}

sic_attr_ptr_t sic_set_prop(sic_node_ptr_t nodus,
                            const sic_char_t *nomen,
                            const sic_char_t *valor)
{
    if (!nodus || !nomen) return NULL;

    /* quaere existentem */
    for (sic_attr_ptr_t a = nodus->properties; a; a = a->next) {
        if (!a->ns &&
            strcmp((const char *)a->name, (const char *)nomen) == 0) {
            free(a->value);
            a->value = valor ? dup_str(valor) : NULL;
            return a;
        }
    }

    /* crea novum */
    sic_attr_ptr_t a = (sic_attr_ptr_t)calloc(1, sizeof(sic_attr_t));
    a->type  = SIC_ATTRIBUTE_NODE;
    a->name  = dup_str(nomen);
    a->value = valor ? dup_str(valor) : NULL;
    attr_append(nodus, a);
    return a;
}

sic_attr_ptr_t sic_set_ns_prop(sic_node_ptr_t nodus, sic_ns_ptr_t ns,
                               const sic_char_t *nomen,
                               const sic_char_t *valor)
{
    if (!nodus || !nomen) return NULL;

    for (sic_attr_ptr_t a = nodus->properties; a; a = a->next) {
        if (strcmp((const char *)a->name, (const char *)nomen) == 0 &&
            a->ns == ns) {
            free(a->value);
            a->value = valor ? dup_str(valor) : NULL;
            return a;
        }
    }

    sic_attr_ptr_t a = (sic_attr_ptr_t)calloc(1, sizeof(sic_attr_t));
    a->type  = SIC_ATTRIBUTE_NODE;
    a->name  = dup_str(nomen);
    a->value = valor ? dup_str(valor) : NULL;
    a->ns    = ns;
    attr_append(nodus, a);
    return a;
}

int sic_unset_prop(sic_node_ptr_t nodus, const sic_char_t *nomen)
{
    if (!nodus || !nomen) return -1;
    for (sic_attr_ptr_t a = nodus->properties; a; a = a->next) {
        if (!a->ns &&
            strcmp((const char *)a->name, (const char *)nomen) == 0) {
            if (a->prev) a->prev->next = a->next;
            else nodus->properties = a->next;
            if (a->next) a->next->prev = a->prev;
            free((void *)a->name);
            free(a->value);
            free(a);
            return 0;
        }
    }
    return -1;
}

/* ================================================================
 * spatia nominum
 * ================================================================ */

sic_ns_ptr_t sic_new_ns(sic_node_ptr_t nodus,
                        const sic_char_t *href,
                        const sic_char_t *praefixum)
{
    sic_ns_ptr_t ns = (sic_ns_ptr_t)calloc(1, sizeof(sic_ns_t));
    ns->href   = href ? dup_str(href) : NULL;
    ns->prefix = praefixum ? dup_str(praefixum) : NULL;
    if (nodus) {
        ns->next     = nodus->nsDef;
        nodus->nsDef = ns;
    }
    return ns;
}

void sic_set_ns(sic_node_ptr_t nodus, sic_ns_ptr_t ns)
{
    if (nodus) nodus->ns = ns;
}

sic_ns_ptr_t sic_search_ns_by_href(sic_doc_ptr_t doc,
                                   sic_node_ptr_t nodus,
                                   const sic_char_t *href)
{
    (void)doc;
    if (!href) return NULL;
    for (sic_node_ptr_t n = nodus; n; n = n->parent)
        for (sic_ns_ptr_t ns = n->nsDef; ns; ns = ns->next)
            if (ns->href &&
                strcmp((const char *)ns->href, (const char *)href) == 0)
                return ns;
    return NULL;
}

/* ================================================================
 * chordae et contentum
 * ================================================================ */

int sic_strcmp(const sic_char_t *s1, const sic_char_t *s2)
{
    if (s1 == s2) return 0;
    if (!s1) return -1;
    if (!s2) return 1;
    return strcmp((const char *)s1, (const char *)s2);
}

/* collige textum ex omnibus descendentibus */
static void collect_text(sic_node_ptr_t nodus, alveus_t *a)
{
    for (sic_node_ptr_t c = nodus->children; c; c = c->next) {
        if ((c->type == SIC_TEXT_NODE ||
             c->type == SIC_CDATA_SECTION_NODE) && c->content)
            alv_cat(a, (const char *)c->content,
                    strlen((const char *)c->content));
        else if (c->type == SIC_ELEMENT_NODE)
            collect_text(c, a);
    }
}

sic_char_t *sic_node_get_content(sic_node_ptr_t nodus)
{
    if (!nodus) return NULL;
    if (nodus->type == SIC_TEXT_NODE ||
        nodus->type == SIC_CDATA_SECTION_NODE)
        return nodus->content ? dup_str(nodus->content) : NULL;

    alveus_t a;
    alv_init(&a);
    collect_text(nodus, &a);
    return alv_fini(&a);
}

void sic_free(void *ptr)
{
    free(ptr);
}

/* ================================================================
 * dissector — XML in arborem
 * ================================================================ */

typedef struct {
    const char    *cur;
    const char    *finis;
    sic_doc_ptr_t  doc;
    int            vexilla;
} dctx_t;

static int  d_eof(dctx_t *d)      { return d->cur >= d->finis; }
static int  d_peek(dctx_t *d)     { return d_eof(d) ? -1 : (unsigned char)*d->cur; }
static int  d_eat(dctx_t *d)      { return d_eof(d) ? -1 : (unsigned char)*d->cur++; }
static void d_skip_ws(dctx_t *d)  { while (!d_eof(d) && isspace((unsigned char)*d->cur)) d->cur++; }

static int d_looking_at(dctx_t *d, const char *s)
{
    size_t lon = strlen(s);
    return ((size_t)(d->finis - d->cur) >= lon &&
            memcmp(d->cur, s, lon) == 0);
}

static int d_match(dctx_t *d, const char *s)
{
    size_t lon = strlen(s);
    if ((size_t)(d->finis - d->cur) < lon ||
        memcmp(d->cur, s, lon) != 0) return 0;
    d->cur += lon;
    return 1;
}

/* --- nomina XML --- */

static int est_nomen_c(int c, int primus)
{
    if (isalpha(c) || c == '_') return 1;
    if (primus) return 0;
    return (isdigit(c) || c == '-' || c == '.' || c == ':');
}

static sic_char_t *d_nomen(dctx_t *d)
{
    if (d_eof(d) || !est_nomen_c(d_peek(d), 1)) return NULL;
    const char *initium = d->cur;
    while (!d_eof(d) && est_nomen_c(d_peek(d), 0)) d->cur++;
    size_t lon = (size_t)(d->cur - initium);
    sic_char_t *s = (sic_char_t *)malloc(lon + 1);
    memcpy(s, initium, lon);
    s[lon] = '\0';
    return s;
}

/* --- entitates --- */

static void d_entitas(dctx_t *d, alveus_t *a)
{
    d->cur++; /* praeterire '&' */
    if (d_looking_at(d, "amp;"))  { d->cur += 4; alv_push(a, '&');  return; }
    if (d_looking_at(d, "lt;"))   { d->cur += 3; alv_push(a, '<');  return; }
    if (d_looking_at(d, "gt;"))   { d->cur += 3; alv_push(a, '>');  return; }
    if (d_looking_at(d, "quot;")) { d->cur += 5; alv_push(a, '"');  return; }
    if (d_looking_at(d, "apos;")) { d->cur += 5; alv_push(a, '\''); return; }
    if (d_peek(d) == '#') {
        d->cur++;
        unsigned int codex = 0;
        if (d_peek(d) == 'x') {
            d->cur++;
            while (!d_eof(d) && d_peek(d) != ';') {
                int c = d_eat(d);
                if      (c >= '0' && c <= '9') codex = codex * 16 + (unsigned)(c - '0');
                else if (c >= 'a' && c <= 'f') codex = codex * 16 + (unsigned)(c - 'a' + 10);
                else if (c >= 'A' && c <= 'F') codex = codex * 16 + (unsigned)(c - 'A' + 10);
            }
        } else {
            while (!d_eof(d) && d_peek(d) != ';') {
                int c = d_peek(d);
                if (c >= '0' && c <= '9')
                    codex = codex * 10 + (unsigned)(c - '0');
                d->cur++;
            }
        }
        if (!d_eof(d)) d->cur++; /* ';' */
        /* codifica UTF-8 */
        if (codex < 0x80) {
            alv_push(a, (char)codex);
        } else if (codex < 0x800) {
            alv_push(a, (char)(0xC0 | (codex >> 6)));
            alv_push(a, (char)(0x80 | (codex & 0x3F)));
        } else if (codex < 0x10000) {
            alv_push(a, (char)(0xE0 | (codex >> 12)));
            alv_push(a, (char)(0x80 | ((codex >> 6) & 0x3F)));
            alv_push(a, (char)(0x80 | (codex & 0x3F)));
        } else {
            alv_push(a, (char)(0xF0 | (codex >> 18)));
            alv_push(a, (char)(0x80 | ((codex >> 12) & 0x3F)));
            alv_push(a, (char)(0x80 | ((codex >> 6) & 0x3F)));
            alv_push(a, (char)(0x80 | (codex & 0x3F)));
        }
        return;
    }
    /* entitas ignota — emitte '&' ut est */
    alv_push(a, '&');
}

/* --- valor attributi inter apices --- */

static sic_char_t *d_valor_attr(dctx_t *d)
{
    int apex = d_eat(d);
    if (apex != '"' && apex != '\'') return NULL;
    alveus_t a;
    alv_init(&a);
    while (!d_eof(d) && d_peek(d) != apex) {
        if (d_peek(d) == '&')
            d_entitas(d, &a);
        else
            alv_push(&a, (char)d_eat(d));
    }
    if (!d_eof(d)) d->cur++; /* apex claudens */
    return alv_fini(&a);
}

/* --- praeterire commentaria, PI, DOCTYPE --- */

static void d_praeter_commentarium(dctx_t *d)
{
    d->cur += 4; /* "<!--" */
    while (!d_eof(d) && !d_looking_at(d, "-->")) d->cur++;
    if (d_looking_at(d, "-->")) d->cur += 3;
}

static void d_praeter_pi(dctx_t *d)
{
    d->cur += 2; /* "<?" */
    while (!d_eof(d) && !d_looking_at(d, "?>")) d->cur++;
    if (d_looking_at(d, "?>")) d->cur += 2;
}

static void d_praeter_doctype(dctx_t *d)
{
    /* iam post "<!DOCTYPE" */
    int altitudo = 0;
    while (!d_eof(d)) {
        int c = d_peek(d);
        if      (c == '[') { altitudo++; d->cur++; }
        else if (c == ']') { altitudo--; d->cur++; }
        else if (c == '>' && altitudo == 0) { d->cur++; return; }
        else d->cur++;
    }
}

/* --- CDATA --- */

static sic_node_ptr_t d_cdata(dctx_t *d)
{
    d->cur += 9; /* "<![CDATA[" */
    const char *initium = d->cur;
    while (!d_eof(d) && !d_looking_at(d, "]]>")) d->cur++;
    size_t lon = (size_t)(d->cur - initium);
    if (d_looking_at(d, "]]>")) d->cur += 3;

    sic_node_ptr_t n = (sic_node_ptr_t)calloc(1, sizeof(sic_node_t));
    n->type    = SIC_TEXT_NODE;
    n->content = (sic_char_t *)malloc(lon + 1);
    memcpy(n->content, initium, lon);
    n->content[lon] = '\0';
    n->doc = d->doc;
    return n;
}

/* --- nodus textus --- */

static sic_node_ptr_t d_textus(dctx_t *d)
{
    alveus_t a;
    alv_init(&a);
    while (!d_eof(d) && d_peek(d) != '<') {
        if (d_peek(d) == '&')
            d_entitas(d, &a);
        else
            alv_push(&a, (char)d_eat(d));
    }
    if (a.lon == 0) { free(a.data); return NULL; }

    sic_node_ptr_t n = (sic_node_ptr_t)calloc(1, sizeof(sic_node_t));
    n->type    = SIC_TEXT_NODE;
    n->content = alv_fini(&a);
    n->doc     = d->doc;
    return n;
}

/* est chorda tota ex spatiis? */
static int est_vacua(const sic_char_t *s)
{
    if (!s) return 1;
    for (; *s; s++)
        if (!isspace(*s)) return 0;
    return 1;
}

/* quaere spatium nominum per praefixum, sursum per arborem */
static sic_ns_ptr_t find_ns_prefix(sic_node_ptr_t nodus,
                                   const char *praef)
{
    for (sic_node_ptr_t n = nodus; n; n = n->parent)
        for (sic_ns_ptr_t ns = n->nsDef; ns; ns = ns->next) {
            if (!praef && !ns->prefix) return ns;
            if (praef && ns->prefix &&
                strcmp(praef, (const char *)ns->prefix) == 0)
                return ns;
        }
    return NULL;
}

/* --- disseca elementum (post '<' iam consumptum) --- */

static void d_contentum(dctx_t *d, sic_node_ptr_t parens);

/* parens_ctx: parens in arbore, pro resolutione spatiorum nominum */
static sic_node_ptr_t d_elementum(dctx_t *d, sic_node_ptr_t parens_ctx)
{
    sic_char_t *nomen_plenum = d_nomen(d);
    if (!nomen_plenum) return NULL;

    /* lege attributa in indicem temporalem */
    typedef struct { sic_char_t *nom; sic_char_t *val; } par_t;
    par_t paria[256];
    int n_paria = 0;

    while (!d_eof(d)) {
        d_skip_ws(d);
        int c = d_peek(d);
        if (c == '/' || c == '>') break;
        sic_char_t *an = d_nomen(d);
        if (!an) break;
        d_skip_ws(d);
        if (d_peek(d) == '=') d->cur++;
        d_skip_ws(d);
        sic_char_t *av = d_valor_attr(d);
        if (n_paria < 256) {
            paria[n_paria].nom = an;
            paria[n_paria].val = av;
            n_paria++;
        } else {
            free(an);
            free(av);
        }
    }

    /* crea nodum */
    sic_node_ptr_t nodus = (sic_node_ptr_t)calloc(1, sizeof(sic_node_t));
    nodus->type   = SIC_ELEMENT_NODE;
    nodus->doc    = d->doc;
    nodus->parent = parens_ctx; /* temporarie, pro resolutione ns */

    /* primo transitu: processa declarationes spatiorum nominum */
    for (int i = 0; i < n_paria; i++) {
        const char *n = (const char *)paria[i].nom;
        if (strcmp(n, "xmlns") == 0) {
            sic_ns_ptr_t ns = (sic_ns_ptr_t)calloc(1, sizeof(sic_ns_t));
            ns->href     = paria[i].val;
            ns->prefix   = NULL;
            ns->next     = nodus->nsDef;
            nodus->nsDef = ns;
            free(paria[i].nom);
            paria[i].nom = NULL;
        } else if (strncmp(n, "xmlns:", 6) == 0) {
            sic_ns_ptr_t ns = (sic_ns_ptr_t)calloc(1, sizeof(sic_ns_t));
            ns->href     = paria[i].val;
            ns->prefix   = dup_str((const sic_char_t *)(n + 6));
            ns->next     = nodus->nsDef;
            nodus->nsDef = ns;
            free(paria[i].nom);
            paria[i].nom = NULL;
        }
    }

    /* resolve nomen elementi */
    char *colon = strchr((char *)nomen_plenum, ':');
    if (colon) {
        *colon = '\0';
        nodus->name = dup_str((const sic_char_t *)(colon + 1));
        nodus->ns   = find_ns_prefix(nodus, (const char *)nomen_plenum);
        free(nomen_plenum);
    } else {
        nodus->name = nomen_plenum;
        nodus->ns   = find_ns_prefix(nodus, NULL);
    }

    /* secundo transitu: crea attributa */
    for (int i = 0; i < n_paria; i++) {
        if (!paria[i].nom) continue;
        sic_attr_ptr_t a = (sic_attr_ptr_t)calloc(1, sizeof(sic_attr_t));
        a->type   = SIC_ATTRIBUTE_NODE;
        a->parent = nodus;
        a->doc    = d->doc;

        colon = strchr((char *)paria[i].nom, ':');
        if (colon) {
            *colon = '\0';
            a->ns   = find_ns_prefix(nodus,
                                     (const char *)paria[i].nom);
            a->name = dup_str((const sic_char_t *)(colon + 1));
            free(paria[i].nom);
        } else {
            a->name = paria[i].nom;
        }
        a->value = paria[i].val;
        attr_append(nodus, a);
    }

    /* elementum clausum vel apertum? */
    if (d_match(d, "/>"))
        return nodus;
    if (!d_eof(d)) d->cur++; /* '>' */

    /* disseca filios */
    d_contentum(d, nodus);

    /* lege signum claudens </nomen> */
    if (d_match(d, "</")) {
        sic_char_t *claudens = d_nomen(d);
        free(claudens);
        d_skip_ws(d);
        if (!d_eof(d) && d_peek(d) == '>') d->cur++;
    }

    return nodus;
}

/* disseca contentum inter signa aperientia et claudentia */
static void d_contentum(dctx_t *d, sic_node_ptr_t parens)
{
    while (!d_eof(d)) {
        if (d_peek(d) == '<') {
            if (d_looking_at(d, "</"))
                return;
            if (d_looking_at(d, "<!--")) {
                d_praeter_commentarium(d);
                continue;
            }
            if (d_looking_at(d, "<![CDATA[")) {
                sic_node_ptr_t c = d_cdata(d);
                if (c) link_child(parens, c);
                continue;
            }
            if (d_looking_at(d, "<?")) {
                d_praeter_pi(d);
                continue;
            }
            /* elementum filium */
            d->cur++; /* '<' */
            sic_node_ptr_t c = d_elementum(d, parens);
            if (c) link_child(parens, c);
        } else {
            sic_node_ptr_t t = d_textus(d);
            if (t) {
                if ((d->vexilla & SIC_PARSE_NOBLANKS) &&
                    est_vacua(t->content))
                    free_node_r(t);
                else
                    link_child(parens, t);
            }
        }
    }
}

/* disseca declarationem XML <?xml ...?> */
static void d_xml_decl(dctx_t *d)
{
    if (!d_looking_at(d, "<?xml")) return;
    d->cur += 5;
    while (!d_eof(d) && !d_looking_at(d, "?>")) {
        d_skip_ws(d);
        if (d_looking_at(d, "?>")) break;
        sic_char_t *nom = d_nomen(d);
        d_skip_ws(d);
        if (!d_eof(d) && d_peek(d) == '=') d->cur++;
        d_skip_ws(d);
        sic_char_t *val = d_valor_attr(d);
        if (nom && val) {
            if (strcmp((const char *)nom, "version") == 0) {
                free(d->doc->version);
                d->doc->version = val;
                val = NULL;
            } else if (strcmp((const char *)nom, "encoding") == 0) {
                free(d->doc->encoding);
                d->doc->encoding = val;
                val = NULL;
            } else if (strcmp((const char *)nom, "standalone") == 0) {
                d->doc->standalone =
                    (strcmp((const char *)val, "yes") == 0);
            }
        }
        free(nom);
        free(val);
    }
    if (d_looking_at(d, "?>")) d->cur += 2;
}

/* --- lege plicam XML --- */

sic_doc_ptr_t sic_read_file(const char *via,
                            const char *codificatio,
                            int optiones)
{
    (void)codificatio;
    FILE *fp = fopen(via, "rb");
    if (!fp) return NULL;

    fseek(fp, 0, SEEK_END);
    long magnitudo = ftell(fp);
    if (magnitudo < 0) { fclose(fp); return NULL; }
    fseek(fp, 0, SEEK_SET);

    char *alveus = (char *)malloc((size_t)magnitudo + 1);
    if (!alveus) { fclose(fp); return NULL; }
    size_t lectum = fread(alveus, 1, (size_t)magnitudo, fp);
    alveus[lectum] = '\0';
    fclose(fp);

    sic_doc_ptr_t doc = (sic_doc_ptr_t)calloc(1, sizeof(sic_doc_t));
    doc->type = SIC_DOCUMENT_NODE;

    const char *c = alveus;
    /* praeterire BOM UTF-8 */
    if (lectum >= 3 &&
        (unsigned char)c[0] == 0xEF &&
        (unsigned char)c[1] == 0xBB &&
        (unsigned char)c[2] == 0xBF)
        c += 3;

    dctx_t ctx = { c, alveus + lectum, doc, optiones };

    d_skip_ws(&ctx);
    d_xml_decl(&ctx);

    /* praeterire miscella ante radicem */
    while (!d_eof(&ctx)) {
        d_skip_ws(&ctx);
        if (d_looking_at(&ctx, "<!--"))      { d_praeter_commentarium(&ctx); continue; }
        if (d_looking_at(&ctx, "<?"))        { d_praeter_pi(&ctx); continue; }
        if (d_looking_at(&ctx, "<!DOCTYPE")) { ctx.cur += 9; d_praeter_doctype(&ctx); continue; }
        break;
    }

    /* disseca radicem */
    if (!d_eof(&ctx) && d_peek(&ctx) == '<') {
        ctx.cur++;
        sic_node_ptr_t radix = d_elementum(&ctx, NULL);
        if (radix) {
            doc->children = radix;
            doc->last     = radix;
            set_doc_r(radix, doc);
        }
    }

    free(alveus);
    if (!doc->version)
        doc->version = dup_str((const sic_char_t *)"1.0");

    return doc;
}

/* ================================================================
 * scriptor — arbor in XML
 * ================================================================ */

static void w_escaped(FILE *fp, const sic_char_t *s, int attr)
{
    if (!s) return;
    for (; *s; s++) {
        switch (*s) {
        case '&': fputs("&amp;", fp);  break;
        case '<': fputs("&lt;", fp);   break;
        case '>': if (!attr) fputs("&gt;", fp); else fputc(*s, fp); break;
        case '"': if (attr) fputs("&quot;", fp); else fputc(*s, fp); break;
        default:  fputc(*s, fp);       break;
        }
    }
}

static void w_nodus(FILE *fp, sic_node_ptr_t nodus, int altitudo,
                    int forma)
{
    if (!nodus) return;

    if (nodus->type == SIC_TEXT_NODE ||
        nodus->type == SIC_CDATA_SECTION_NODE) {
        w_escaped(fp, nodus->content, 0);
        return;
    }
    if (nodus->type != SIC_ELEMENT_NODE) return;

    if (forma && altitudo > 0) {
        fputc('\n', fp);
        for (int i = 0; i < altitudo; i++) fputs("  ", fp);
    }

    fputc('<', fp);
    if (nodus->ns && nodus->ns->prefix)
        fprintf(fp, "%s:", nodus->ns->prefix);
    fputs((const char *)nodus->name, fp);

    /* declarationes spatiorum nominum */
    for (sic_ns_ptr_t ns = nodus->nsDef; ns; ns = ns->next) {
        if (ns->prefix)
            fprintf(fp, " xmlns:%s=\"", ns->prefix);
        else
            fputs(" xmlns=\"", fp);
        w_escaped(fp, ns->href, 1);
        fputc('"', fp);
    }

    /* attributa */
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

    /* habet filios elementorum? */
    int habet_elem = 0;
    for (sic_node_ptr_t c = nodus->children; c; c = c->next)
        if (c->type == SIC_ELEMENT_NODE) { habet_elem = 1; break; }

    for (sic_node_ptr_t c = nodus->children; c; c = c->next)
        w_nodus(fp, c, altitudo + 1, forma && habet_elem);

    if (forma && habet_elem) {
        fputc('\n', fp);
        for (int i = 0; i < altitudo; i++) fputs("  ", fp);
    }

    fputs("</", fp);
    if (nodus->ns && nodus->ns->prefix)
        fprintf(fp, "%s:", nodus->ns->prefix);
    fprintf(fp, "%s>", nodus->name);
}

void sic_save_format_fp(FILE *fp, sic_doc_ptr_t doc,
                        const char *codificatio, int forma)
{
    if (!fp || !doc) return;

    fprintf(fp, "<?xml version=\"%s\"",
        doc->version ? (const char *)doc->version : "1.0");
    if (codificatio)
        fprintf(fp, " encoding=\"%s\"", codificatio);
    else if (doc->encoding)
        fprintf(fp, " encoding=\"%s\"", (const char *)doc->encoding);
    fputs("?>\n", fp);

    for (sic_node_ptr_t c = doc->children; c; c = c->next)
        w_nodus(fp, c, 0, forma);
    if (forma) fputc('\n', fp);
}

int sic_save_format_file_enc(const char *via, sic_doc_ptr_t doc,
                             const char *codificatio, int forma)
{
    if (!via || !doc) return -1;
    FILE *fp = fopen(via, "w");
    if (!fp) return -1;
    sic_save_format_fp(fp, doc, codificatio, forma);
    fclose(fp);
    return 0;
}
