/*
 * proba.c — probatio bibliothecae SIC
 *
 * Exercet omnes functiones quas nexcel.example adhibet:
 *   sic_read_file, sic_new_doc, sic_free_doc,
 *   sic_doc_get_root_element, sic_doc_set_root_element,
 *   sic_new_node, sic_new_child, sic_new_text, sic_new_text_child,
 *   sic_add_child, sic_add_prev_sibling,
 *   sic_unlink_node, sic_free_node,
 *   sic_set_prop, sic_get_prop, sic_unset_prop,
 *   sic_set_ns_prop, sic_get_ns_prop,
 *   sic_new_ns, sic_set_ns, sic_search_ns_by_href,
 *   sic_strcmp, sic_node_get_content, sic_free,
 *   sic_save_format_file_enc,
 *   structurae: node->children, node->next, node->type, node->name
 */
#include "sic.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SC (const sic_char_t *)
static int num_probationes = 0;
static int num_rectae      = 0;

static void affirma(int cond, const char *nomen)
{
    num_probationes++;
    if (cond) {
        num_rectae++;
    } else {
        fprintf(stderr, "  FALSUM: %s\n", nomen);
    }
}

/* ================================================================
 * I. sic_new_doc, sic_doc_set_root_element, sic_new_node,
 *    sic_new_ns, sic_set_ns, sic_new_child, sic_set_prop,
 *    sic_set_ns_prop, sic_save_format_file_enc, sic_free_doc
 * ================================================================ */
static void proba_crea_documentum(void)
{
    printf("  I. crea documentum...\n");

    sic_doc_ptr_t doc = sic_new_doc(SC "1.0");
    affirma(doc != NULL, "sic_new_doc");

    /* sic_new_node + sic_doc_set_root_element */
    sic_node_ptr_t radix = sic_new_node(NULL, SC "workbook");
    sic_node_ptr_t vetus = sic_doc_set_root_element(doc, radix);
    affirma(vetus == NULL, "sic_doc_set_root_element (prima vice)");
    affirma(sic_doc_get_root_element(doc) == radix,
        "sic_doc_get_root_element");

    /* sic_new_ns (praefinitum + praefixum) */
    sic_ns_ptr_t ns_sml = sic_new_ns(radix,
        SC "http://schemas.example.com/spreadsheetml/main", NULL);
    sic_ns_ptr_t ns_r = sic_new_ns(radix,
        SC "http://schemas.example.com/relationships", SC "r");
    affirma(ns_sml != NULL, "sic_new_ns praefinitum");
    affirma(ns_r != NULL, "sic_new_ns praefixum");

    /* sic_set_ns */
    sic_set_ns(radix, ns_sml);
    affirma(radix->ns == ns_sml, "sic_set_ns");

    /* sic_new_child (sine contentum) */
    sic_node_ptr_t sheets = sic_new_child(radix, NULL,
        SC "sheets", NULL);
    affirma(sheets != NULL, "sic_new_child sine contentum");
    affirma(sheets->parent == radix, "filius->parent");

    sic_node_ptr_t sh1 = sic_new_child(sheets, NULL,
        SC "sheet", NULL);
    sic_node_ptr_t sh2 = sic_new_child(sheets, NULL,
        SC "sheet", NULL);

    /* sic_set_prop */
    sic_set_prop(sh1, SC "name", SC "Sheet1");
    sic_set_prop(sh1, SC "sheetId", SC "1");
    sic_set_prop(sh2, SC "name", SC "Sheet2");
    sic_set_prop(sh2, SC "sheetId", SC "2");

    /* sic_set_ns_prop */
    sic_set_ns_prop(sh1, ns_r, SC "id", SC "rId1");
    sic_set_ns_prop(sh2, ns_r, SC "id", SC "rId2");

    /* sic_save_format_file_enc */
    int res = sic_save_format_file_enc(
        "/tmp/sic_proba1.xml", doc, "UTF-8", 1);
    affirma(res == 0, "sic_save_format_file_enc");

    sic_free_doc(doc);
}

/* ================================================================
 * II. sic_read_file, navigatio per children/next/type/name,
 *     sic_get_prop, sic_get_ns_prop, sic_strcmp, sic_free
 * ================================================================ */
static void proba_lege_et_naviga(void)
{
    printf("  II. lege et naviga...\n");

    /* sic_read_file cum vexillis */
    sic_doc_ptr_t doc = sic_read_file("/tmp/sic_proba1.xml", NULL,
        SIC_PARSE_NONET | SIC_PARSE_NOBLANKS |
        SIC_PARSE_RECOVER | SIC_PARSE_NOERROR);
    affirma(doc != NULL, "sic_read_file");

    sic_node_ptr_t radix = sic_doc_get_root_element(doc);
    affirma(radix != NULL, "radix non nulla");

    /* sic_strcmp + node->name */
    affirma(sic_strcmp(radix->name, SC "workbook") == 0,
        "radix->name == workbook");

    /* navigatio per node->children, node->next, node->type */
    sic_node_ptr_t sheets = NULL;
    for (sic_node_ptr_t c = radix->children; c; c = c->next)
        if (c->type == SIC_ELEMENT_NODE &&
            sic_strcmp(c->name, SC "sheets") == 0)
        { sheets = c; break; }
    affirma(sheets != NULL, "inveni sheets");

    /* sic_get_prop */
    sic_node_ptr_t sh1 = NULL;
    for (sic_node_ptr_t c = sheets->children; c; c = c->next)
        if (c->type == SIC_ELEMENT_NODE) { sh1 = c; break; }
    affirma(sh1 != NULL, "inveni primum sheet");

    sic_char_t *nom = sic_get_prop(sh1, SC "name");
    affirma(nom != NULL && strcmp((const char *)nom, "Sheet1") == 0,
        "sic_get_prop name");
    sic_free(nom);

    sic_char_t *sid = sic_get_prop(sh1, SC "sheetId");
    affirma(sid != NULL && strcmp((const char *)sid, "1") == 0,
        "sic_get_prop sheetId");
    sic_free(sid);

    /* sic_get_ns_prop */
    sic_char_t *rid = sic_get_ns_prop(sh1, SC "id",
        SC "http://schemas.example.com/relationships");
    affirma(rid != NULL && strcmp((const char *)rid, "rId1") == 0,
        "sic_get_ns_prop r:id");
    sic_free(rid);

    /* sic_get_prop reddit NULL si non adest */
    sic_char_t *nex = sic_get_prop(sh1, SC "nonexistens");
    affirma(nex == NULL, "sic_get_prop non existens");

    sic_free_doc(doc);
}

/* ================================================================
 * III. sic_search_ns_by_href
 * ================================================================ */
static void proba_search_ns(void)
{
    printf("  III. search_ns_by_href...\n");

    sic_doc_ptr_t doc = sic_read_file("/tmp/sic_proba1.xml", NULL,
        SIC_PARSE_NOBLANKS);
    sic_node_ptr_t radix = sic_doc_get_root_element(doc);

    /* quaere ns in nodo ubi declarata est */
    sic_ns_ptr_t ns = sic_search_ns_by_href(doc, radix,
        SC "http://schemas.example.com/relationships");
    affirma(ns != NULL, "search_ns_by_href in radice");
    affirma(ns->prefix != NULL &&
        strcmp((const char *)ns->prefix, "r") == 0,
        "ns->prefix == r");

    /* quaere ns in descendente (debet sursum per arborem ascendere) */
    sic_node_ptr_t sheets = radix->children;
    affirma(sheets != NULL, "sheets pro ns search");
    sic_ns_ptr_t ns2 = sic_search_ns_by_href(doc, sheets,
        SC "http://schemas.example.com/relationships");
    affirma(ns2 == ns, "search_ns_by_href sursum per arborem");

    /* quaere ns praefinitum */
    sic_ns_ptr_t ns_def = sic_search_ns_by_href(doc, radix,
        SC "http://schemas.example.com/spreadsheetml/main");
    affirma(ns_def != NULL, "search_ns_by_href praefinitum");
    affirma(ns_def->prefix == NULL, "ns praefinitum sine praefixo");

    sic_free_doc(doc);
}

/* ================================================================
 * IV. sic_unlink_node, sic_free_node, sic_add_child,
 *     sic_add_prev_sibling
 * ================================================================ */
static void proba_manipula_arborem(void)
{
    printf("  IV. manipula arborem...\n");

    sic_doc_ptr_t doc = sic_new_doc(SC "1.0");
    sic_node_ptr_t radix = sic_new_node(NULL, SC "r");
    sic_doc_set_root_element(doc, radix);

    sic_node_ptr_t a = sic_new_child(radix, NULL, SC "a", NULL);
    sic_node_ptr_t b = sic_new_child(radix, NULL, SC "b", NULL);
    sic_node_ptr_t c = sic_new_child(radix, NULL, SC "c", NULL);

    /* sic_unlink_node — tolle medium */
    sic_unlink_node(b);
    affirma(a->next == c, "post unlink b: a->next == c");
    affirma(c->prev == a, "post unlink b: c->prev == a");
    affirma(b->parent == NULL, "b sine parente post unlink");

    /* sic_free_node */
    sic_free_node(b);

    /* sic_add_prev_sibling — insere ante c */
    sic_node_ptr_t d = sic_new_node(NULL, SC "d");
    sic_add_prev_sibling(c, d);
    affirma(a->next == d, "post add_prev_sibling: a->next == d");
    affirma(d->next == c, "post add_prev_sibling: d->next == c");
    affirma(c->prev == d, "post add_prev_sibling: c->prev == d");
    affirma(d->parent == radix, "d->parent == radix");

    /* sic_add_prev_sibling — insere ante primum */
    sic_node_ptr_t e = sic_new_node(NULL, SC "e");
    sic_add_prev_sibling(a, e);
    affirma(radix->children == e, "e est primus filius");
    affirma(e->next == a, "e->next == a");

    /* sic_add_child — adde ad finem */
    sic_node_ptr_t f = sic_new_node(NULL, SC "f");
    sic_add_child(radix, f);
    affirma(radix->last == f, "f est ultimus filius");
    affirma(c->next == f, "c->next == f");

    /* sic_add_child cum nodo qui iam parentem habet */
    sic_node_ptr_t sub = sic_new_node(NULL, SC "sub");
    sic_node_ptr_t g = sic_new_child(sub, NULL, SC "g", NULL);
    sic_add_child(radix, g); /* debet g ab sub amovere */
    affirma(g->parent == radix, "g translatum ad radix");
    affirma(sub->children == NULL, "sub sine filiis");
    sic_free_node(sub);

    sic_free_doc(doc);
}

/* ================================================================
 * V. sic_new_text, sic_add_child, sic_node_get_content —
 *    pattern nxl_xml_set_text
 * ================================================================ */
static void proba_textus(void)
{
    printf("  V. textus et contentum...\n");

    sic_doc_ptr_t doc = sic_new_doc(SC "1.0");
    sic_node_ptr_t radix = sic_new_node(NULL, SC "r");
    sic_doc_set_root_element(doc, radix);

    /* sic_new_text_child — crea elementum cum textu */
    sic_node_ptr_t v = sic_new_text_child(radix, NULL,
        SC "v", SC "3.14");
    sic_char_t *cont = sic_node_get_content(v);
    affirma(cont != NULL && strcmp((const char *)cont, "3.14") == 0,
        "sic_new_text_child + sic_node_get_content");
    sic_free(cont);

    /* sic_new_text + sic_add_child */
    sic_node_ptr_t elem = sic_new_child(radix, NULL, SC "t", NULL);
    sic_node_ptr_t txt = sic_new_text(SC "hello");
    sic_add_child(elem, txt);
    cont = sic_node_get_content(elem);
    affirma(cont != NULL && strcmp((const char *)cont, "hello") == 0,
        "sic_new_text + sic_add_child");
    sic_free(cont);

    /* pattern nxl_xml_set_text: dele filios, adde novum textum */
    sic_node_ptr_t ch;
    while ((ch = elem->children) != NULL) {
        sic_unlink_node(ch);
        sic_free_node(ch);
    }
    affirma(elem->children == NULL, "post deletionem filiorum");
    sic_add_child(elem, sic_new_text(SC "world"));
    cont = sic_node_get_content(elem);
    affirma(cont != NULL && strcmp((const char *)cont, "world") == 0,
        "nxl_xml_set_text pattern");
    sic_free(cont);

    /* sic_node_get_content per descendentes profundos */
    sic_node_ptr_t si = sic_new_child(radix, NULL, SC "si", NULL);
    sic_new_text_child(si, NULL, SC "t", SC "hello ");
    sic_new_text_child(si, NULL, SC "t", SC "world");
    cont = sic_node_get_content(si);
    affirma(cont != NULL &&
        strcmp((const char *)cont, "hello world") == 0,
        "sic_node_get_content recursivum");
    sic_free(cont);

    /* sic_node_get_content in nodo textus */
    sic_node_ptr_t tn = sic_new_text(SC "directum");
    cont = sic_node_get_content(tn);
    affirma(cont != NULL &&
        strcmp((const char *)cont, "directum") == 0,
        "sic_node_get_content nodi textus");
    sic_free(cont);
    sic_free_node(tn);

    sic_free_doc(doc);
}

/* ================================================================
 * VI. sic_set_prop (superpone), sic_unset_prop, sic_set_ns_prop
 * ================================================================ */
static void proba_proprietates(void)
{
    printf("  VI. proprietates...\n");

    sic_doc_ptr_t doc = sic_new_doc(SC "1.0");
    sic_node_ptr_t radix = sic_new_node(NULL, SC "r");
    sic_doc_set_root_element(doc, radix);

    /* sic_set_prop — crea */
    sic_set_prop(radix, SC "x", SC "1");
    sic_char_t *v = sic_get_prop(radix, SC "x");
    affirma(v != NULL && strcmp((const char *)v, "1") == 0,
        "sic_set_prop crea");
    sic_free(v);

    /* sic_set_prop — superpone */
    sic_set_prop(radix, SC "x", SC "2");
    v = sic_get_prop(radix, SC "x");
    affirma(v != NULL && strcmp((const char *)v, "2") == 0,
        "sic_set_prop superpone");
    sic_free(v);

    /* plura attributa */
    sic_set_prop(radix, SC "y", SC "3");
    sic_set_prop(radix, SC "z", SC "4");
    v = sic_get_prop(radix, SC "y");
    affirma(v != NULL && strcmp((const char *)v, "3") == 0,
        "plura attributa y");
    sic_free(v);
    v = sic_get_prop(radix, SC "z");
    affirma(v != NULL && strcmp((const char *)v, "4") == 0,
        "plura attributa z");
    sic_free(v);

    /* sic_unset_prop */
    affirma(sic_unset_prop(radix, SC "y") == 0,
        "sic_unset_prop rectum");
    affirma(sic_get_prop(radix, SC "y") == NULL,
        "post unset: non adest");
    affirma(sic_unset_prop(radix, SC "nonexistens") == -1,
        "sic_unset_prop non existens");

    /* sic_set_ns_prop + sic_get_ns_prop */
    sic_ns_ptr_t ns = sic_new_ns(radix, SC "http://ns.example", SC "p");
    sic_set_ns_prop(radix, ns, SC "attr", SC "nsval");
    sic_char_t *nv = sic_get_ns_prop(radix, SC "attr",
        SC "http://ns.example");
    affirma(nv != NULL && strcmp((const char *)nv, "nsval") == 0,
        "sic_set_ns_prop + sic_get_ns_prop");
    sic_free(nv);

    /* sic_set_ns_prop — superpone */
    sic_set_ns_prop(radix, ns, SC "attr", SC "nsval2");
    nv = sic_get_ns_prop(radix, SC "attr", SC "http://ns.example");
    affirma(nv != NULL && strcmp((const char *)nv, "nsval2") == 0,
        "sic_set_ns_prop superpone");
    sic_free(nv);

    /* attributum sine ns et cum ns eodem nomine non confliguntur */
    sic_set_prop(radix, SC "attr", SC "plain");
    v = sic_get_prop(radix, SC "attr");
    affirma(v != NULL && strcmp((const char *)v, "plain") == 0,
        "attr sine ns non confligit");
    sic_free(v);
    nv = sic_get_ns_prop(radix, SC "attr", SC "http://ns.example");
    affirma(nv != NULL && strcmp((const char *)nv, "nsval2") == 0,
        "attr cum ns non confligit");
    sic_free(nv);

    sic_free_doc(doc);
}

/* ================================================================
 * VII. entitates et circuitus scriptor/dissector
 * ================================================================ */
static void proba_entitates(void)
{
    printf("  VII. entitates...\n");

    sic_doc_ptr_t doc = sic_new_doc(SC "1.0");
    sic_node_ptr_t radix = sic_new_node(NULL, SC "r");
    sic_doc_set_root_element(doc, radix);

    /* characteres qui effugiendi sunt */
    sic_set_prop(radix, SC "a", SC "a&b<c\"d'e");
    sic_new_text_child(radix, NULL, SC "t", SC "x&y<z>w");

    sic_save_format_file_enc("/tmp/sic_ent.xml", doc, "UTF-8", 0);
    sic_free_doc(doc);

    /* relege et verifica circuitum */
    doc = sic_read_file("/tmp/sic_ent.xml", NULL, 0);
    affirma(doc != NULL, "relege entitates");
    radix = sic_doc_get_root_element(doc);

    sic_char_t *av = sic_get_prop(radix, SC "a");
    affirma(av != NULL && strcmp((const char *)av, "a&b<c\"d'e") == 0,
        "circuitus entitatum in attributo");
    sic_free(av);

    sic_node_ptr_t t = radix->children;
    affirma(t != NULL && t->type == SIC_ELEMENT_NODE,
        "elementum t adest");
    sic_char_t *tc = sic_node_get_content(t);
    affirma(tc != NULL && strcmp((const char *)tc, "x&y<z>w") == 0,
        "circuitus entitatum in textu");
    sic_free(tc);

    sic_free_doc(doc);
}

/* ================================================================
 * VIII. SIC_PARSE_NOBLANKS — praetermitte textus vacuos
 * ================================================================ */
static void proba_noblanks(void)
{
    printf("  VIII. NOBLANKS...\n");

    /* scribe XML cum spatiis inter elementa */
    sic_doc_ptr_t doc = sic_new_doc(SC "1.0");
    sic_node_ptr_t radix = sic_new_node(NULL, SC "r");
    sic_doc_set_root_element(doc, radix);
    sic_new_child(radix, NULL, SC "a", NULL);
    sic_new_child(radix, NULL, SC "b", NULL);
    sic_save_format_file_enc("/tmp/sic_nb.xml", doc, "UTF-8", 1);
    sic_free_doc(doc);

    /* lege cum NOBLANKS — nulla nodi textus vacui */
    doc = sic_read_file("/tmp/sic_nb.xml", NULL, SIC_PARSE_NOBLANKS);
    radix = sic_doc_get_root_element(doc);
    int textus_vacui = 0;
    for (sic_node_ptr_t c = radix->children; c; c = c->next)
        if (c->type == SIC_TEXT_NODE) textus_vacui++;
    affirma(textus_vacui == 0,
        "NOBLANKS: nulli nodi textus vacui");

    /* verifica elementa praesentia */
    int elementa = 0;
    for (sic_node_ptr_t c = radix->children; c; c = c->next)
        if (c->type == SIC_ELEMENT_NODE) elementa++;
    affirma(elementa == 2, "NOBLANKS: duo elementa praesentia");
    sic_free_doc(doc);

    /* lege sine NOBLANKS — textus vacui adsunt */
    doc = sic_read_file("/tmp/sic_nb.xml", NULL, 0);
    radix = sic_doc_get_root_element(doc);
    textus_vacui = 0;
    for (sic_node_ptr_t c = radix->children; c; c = c->next)
        if (c->type == SIC_TEXT_NODE) textus_vacui++;
    affirma(textus_vacui > 0,
        "sine NOBLANKS: textus vacui adsunt");
    sic_free_doc(doc);
}

/* ================================================================
 * IX. sic_doc_set_root_element — substitutio radicis
 * ================================================================ */
static void proba_substitue_radicem(void)
{
    printf("  IX. substitue radicem...\n");

    sic_doc_ptr_t doc = sic_new_doc(SC "1.0");
    sic_node_ptr_t r1 = sic_new_node(NULL, SC "old");
    sic_doc_set_root_element(doc, r1);

    sic_node_ptr_t r2 = sic_new_node(NULL, SC "new");
    sic_node_ptr_t vetus = sic_doc_set_root_element(doc, r2);
    affirma(vetus == r1, "vetus radix reddita");
    affirma(sic_doc_get_root_element(doc) == r2,
        "nova radix instituta");

    sic_free_node(r1); /* vetus iam non in arbore */
    sic_free_doc(doc);
}

/* ================================================================
 * X. spatia nominum — multi-level, scriptor/dissector circuitus
 * ================================================================ */
static void proba_ns_complexa(void)
{
    printf("  X. spatia nominum complexa...\n");

    /* aedifica documentum cum pluribus spatiis nominum */
    sic_doc_ptr_t doc = sic_new_doc(SC "1.0");
    sic_node_ptr_t radix = sic_new_node(NULL, SC "Relationships");
    sic_doc_set_root_element(doc, radix);

    sic_ns_ptr_t ns_pkg = sic_new_ns(radix,
        SC "http://schemas.example.com/package/relationships", NULL);
    sic_set_ns(radix, ns_pkg);

    sic_node_ptr_t rel = sic_new_child(radix, NULL,
        SC "Relationship", NULL);
    sic_set_prop(rel, SC "Id", SC "rId1");
    sic_set_prop(rel, SC "Type",
        SC "http://schemas.example.com/officeDocument");
    sic_set_prop(rel, SC "Target", SC "xl/workbook.xml");

    sic_save_format_file_enc("/tmp/sic_ns.xml", doc, "UTF-8", 1);
    sic_free_doc(doc);

    /* relege et verifica */
    doc = sic_read_file("/tmp/sic_ns.xml", NULL, SIC_PARSE_NOBLANKS);
    radix = sic_doc_get_root_element(doc);
    affirma(sic_strcmp(radix->name, SC "Relationships") == 0,
        "ns: nomen radicis");

    /* search_ns_by_href in filio */
    sic_node_ptr_t ch = radix->children;
    affirma(ch != NULL && ch->type == SIC_ELEMENT_NODE,
        "ns: filius adest");

    sic_ns_ptr_t found = sic_search_ns_by_href(doc, ch,
        SC "http://schemas.example.com/package/relationships");
    affirma(found != NULL, "search_ns_by_href per filium");

    sic_char_t *tgt = sic_get_prop(ch, SC "Target");
    affirma(tgt != NULL &&
        strcmp((const char *)tgt, "xl/workbook.xml") == 0,
        "ns: attributum Target");
    sic_free(tgt);

    sic_free_doc(doc);
}

/* ================================================================
 * XI. sic_read_file — plica non existens
 * ================================================================ */
static void proba_plica_nulla(void)
{
    printf("  XI. plica non existens...\n");
    sic_doc_ptr_t doc = sic_read_file(
        "/tmp/sic_non_existens_12345.xml", NULL, 0);
    affirma(doc == NULL, "sic_read_file reddit NULL pro plica nulla");
}

/* ================================================================
 * XII. sic_strcmp — casus limites
 * ================================================================ */
static void proba_strcmp(void)
{
    printf("  XII. sic_strcmp...\n");
    affirma(sic_strcmp(SC "abc", SC "abc") == 0, "strcmp aequales");
    affirma(sic_strcmp(SC "abc", SC "abd") < 0, "strcmp minor");
    affirma(sic_strcmp(SC "abd", SC "abc") > 0, "strcmp maior");
    affirma(sic_strcmp(NULL, SC "a") < 0, "strcmp NULL sinister");
    affirma(sic_strcmp(SC "a", NULL) > 0, "strcmp NULL dexter");
    affirma(sic_strcmp(NULL, NULL) == 0, "strcmp NULL ambo");
}

/* ================================================================
 * principalis
 * ================================================================ */
int main(void)
{
    printf("SIC probationes:\n");
    proba_crea_documentum();
    proba_lege_et_naviga();
    proba_search_ns();
    proba_manipula_arborem();
    proba_textus();
    proba_proprietates();
    proba_entitates();
    proba_noblanks();
    proba_substitue_radicem();
    proba_ns_complexa();
    proba_plica_nulla();
    proba_strcmp();

    /* purga plicas temporales */
    remove("/tmp/sic_proba1.xml");
    remove("/tmp/sic_ent.xml");
    remove("/tmp/sic_nb.xml");
    remove("/tmp/sic_ns.xml");

    printf("\n%d/%d probationes transierunt.\n",
        num_rectae, num_probationes);
    return (num_rectae == num_probationes) ? 0 : 1;
}
