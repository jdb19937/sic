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
    affirma(
        sic_doc_get_root_element(doc) == radix,
        "sic_doc_get_root_element"
    );

    /* sic_new_ns (praefinitum + praefixum) */
    sic_ns_ptr_t ns_sml = sic_new_ns(
        radix,
        SC "http://schemas.example.com/spreadsheetml/main", NULL
    );
    sic_ns_ptr_t ns_r = sic_new_ns(
        radix,
        SC "http://schemas.example.com/relationships", SC "r"
    );
    affirma(ns_sml != NULL, "sic_new_ns praefinitum");
    affirma(ns_r != NULL, "sic_new_ns praefixum");

    /* sic_set_ns */
    sic_set_ns(radix, ns_sml);
    affirma(radix->ns == ns_sml, "sic_set_ns");

    /* sic_new_child (sine contentum) */
    sic_node_ptr_t sheets = sic_new_child(
        radix, NULL,
        SC "sheets", NULL
    );
    affirma(sheets != NULL, "sic_new_child sine contentum");
    affirma(sheets->parent == radix, "filius->parent");

    sic_node_ptr_t sh1 = sic_new_child(
        sheets, NULL,
        SC "sheet", NULL
    );
    sic_node_ptr_t sh2 = sic_new_child(
        sheets, NULL,
        SC "sheet", NULL
    );

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
        "artifacta/sic_proba1.xml", doc, "UTF-8", 1
    );
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
    sic_doc_ptr_t doc = sic_read_file(
        "artifacta/sic_proba1.xml", NULL,
        SIC_PARSE_NONET | SIC_PARSE_NOBLANKS |
        SIC_PARSE_RECOVER | SIC_PARSE_NOERROR
    );
    affirma(doc != NULL, "sic_read_file");

    sic_node_ptr_t radix = sic_doc_get_root_element(doc);
    affirma(radix != NULL, "radix non nulla");

    /* sic_strcmp + node->name */
    affirma(
        sic_strcmp(radix->name, SC "workbook") == 0,
        "radix->name == workbook"
    );

    /* navigatio per node->children, node->next, node->type */
    sic_node_ptr_t sheets = NULL;
    for (sic_node_ptr_t c = radix->children; c; c = c->next)
        if (
            c->type == SIC_ELEMENT_NODE &&
            sic_strcmp(c->name, SC "sheets") == 0
        ) {
        sheets = c;
        break;
    }
    affirma(sheets != NULL, "inveni sheets");

    /* sic_get_prop */
    sic_node_ptr_t sh1 = NULL;
    for (sic_node_ptr_t c = sheets->children; c; c = c->next)
        if (c->type == SIC_ELEMENT_NODE) {
        sh1 = c;
        break;
    }
    affirma(sh1 != NULL, "inveni primum sheet");

    sic_char_t *nom = sic_get_prop(sh1, SC "name");
    affirma(
        nom != NULL && strcmp((const char *)nom, "Sheet1") == 0,
        "sic_get_prop name"
    );
    sic_free(nom);

    sic_char_t *sid = sic_get_prop(sh1, SC "sheetId");
    affirma(
        sid != NULL && strcmp((const char *)sid, "1") == 0,
        "sic_get_prop sheetId"
    );
    sic_free(sid);

    /* sic_get_ns_prop */
    sic_char_t *rid = sic_get_ns_prop(
        sh1, SC "id",
        SC "http://schemas.example.com/relationships"
    );
    affirma(
        rid != NULL && strcmp((const char *)rid, "rId1") == 0,
        "sic_get_ns_prop r:id"
    );
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

    sic_doc_ptr_t doc = sic_read_file(
        "artifacta/sic_proba1.xml", NULL,
        SIC_PARSE_NOBLANKS
    );
    sic_node_ptr_t radix = sic_doc_get_root_element(doc);

    /* quaere ns in nodo ubi declarata est */
    sic_ns_ptr_t ns = sic_search_ns_by_href(
        doc, radix,
        SC "http://schemas.example.com/relationships"
    );
    affirma(ns != NULL, "search_ns_by_href in radice");
    affirma(
        ns->prefix != NULL &&
        strcmp((const char *)ns->prefix, "r") == 0,
        "ns->prefix == r"
    );

    /* quaere ns in descendente (debet sursum per arborem ascendere) */
    sic_node_ptr_t sheets = radix->children;
    affirma(sheets != NULL, "sheets pro ns search");
    sic_ns_ptr_t ns2 = sic_search_ns_by_href(
        doc, sheets,
        SC "http://schemas.example.com/relationships"
    );
    affirma(ns2 == ns, "search_ns_by_href sursum per arborem");

    /* quaere ns praefinitum */
    sic_ns_ptr_t ns_def = sic_search_ns_by_href(
        doc, radix,
        SC "http://schemas.example.com/spreadsheetml/main"
    );
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

    sic_doc_ptr_t doc    = sic_new_doc(SC "1.0");
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
    sic_node_ptr_t g   = sic_new_child(sub, NULL, SC "g", NULL);
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

    sic_doc_ptr_t doc    = sic_new_doc(SC "1.0");
    sic_node_ptr_t radix = sic_new_node(NULL, SC "r");
    sic_doc_set_root_element(doc, radix);

    /* sic_new_text_child — crea elementum cum textu */
    sic_node_ptr_t v = sic_new_text_child(
        radix, NULL,
        SC "v", SC "3.14"
    );
    sic_char_t *cont = sic_node_get_content(v);
    affirma(
        cont != NULL && strcmp((const char *)cont, "3.14") == 0,
        "sic_new_text_child + sic_node_get_content"
    );
    sic_free(cont);

    /* sic_new_text + sic_add_child */
    sic_node_ptr_t elem = sic_new_child(radix, NULL, SC "t", NULL);
    sic_node_ptr_t txt  = sic_new_text(SC "hello");
    sic_add_child(elem, txt);
    cont = sic_node_get_content(elem);
    affirma(
        cont != NULL && strcmp((const char *)cont, "hello") == 0,
        "sic_new_text + sic_add_child"
    );
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
    affirma(
        cont != NULL && strcmp((const char *)cont, "world") == 0,
        "nxl_xml_set_text pattern"
    );
    sic_free(cont);

    /* sic_node_get_content per descendentes profundos */
    sic_node_ptr_t si = sic_new_child(radix, NULL, SC "si", NULL);
    sic_new_text_child(si, NULL, SC "t", SC "hello ");
    sic_new_text_child(si, NULL, SC "t", SC "world");
    cont = sic_node_get_content(si);
    affirma(
        cont != NULL &&
        strcmp((const char *)cont, "hello world") == 0,
        "sic_node_get_content recursivum"
    );
    sic_free(cont);

    /* sic_node_get_content in nodo textus */
    sic_node_ptr_t tn = sic_new_text(SC "directum");
    cont = sic_node_get_content(tn);
    affirma(
        cont != NULL &&
        strcmp((const char *)cont, "directum") == 0,
        "sic_node_get_content nodi textus"
    );
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

    sic_doc_ptr_t doc    = sic_new_doc(SC "1.0");
    sic_node_ptr_t radix = sic_new_node(NULL, SC "r");
    sic_doc_set_root_element(doc, radix);

    /* sic_set_prop — crea */
    sic_set_prop(radix, SC "x", SC "1");
    sic_char_t *v = sic_get_prop(radix, SC "x");
    affirma(
        v != NULL && strcmp((const char *)v, "1") == 0,
        "sic_set_prop crea"
    );
    sic_free(v);

    /* sic_set_prop — superpone */
    sic_set_prop(radix, SC "x", SC "2");
    v = sic_get_prop(radix, SC "x");
    affirma(
        v != NULL && strcmp((const char *)v, "2") == 0,
        "sic_set_prop superpone"
    );
    sic_free(v);

    /* plura attributa */
    sic_set_prop(radix, SC "y", SC "3");
    sic_set_prop(radix, SC "z", SC "4");
    v = sic_get_prop(radix, SC "y");
    affirma(
        v != NULL && strcmp((const char *)v, "3") == 0,
        "plura attributa y"
    );
    sic_free(v);
    v = sic_get_prop(radix, SC "z");
    affirma(
        v != NULL && strcmp((const char *)v, "4") == 0,
        "plura attributa z"
    );
    sic_free(v);

    /* sic_unset_prop */
    affirma(
        sic_unset_prop(radix, SC "y") == 0,
        "sic_unset_prop rectum"
    );
    affirma(
        sic_get_prop(radix, SC "y") == NULL,
        "post unset: non adest"
    );
    affirma(
        sic_unset_prop(radix, SC "nonexistens") == -1,
        "sic_unset_prop non existens"
    );

    /* sic_set_ns_prop + sic_get_ns_prop */
    sic_ns_ptr_t ns = sic_new_ns(radix, SC "http://ns.example", SC "p");
    sic_set_ns_prop(radix, ns, SC "attr", SC "nsval");
    sic_char_t *nv = sic_get_ns_prop(
        radix, SC "attr",
        SC "http://ns.example"
    );
    affirma(
        nv != NULL && strcmp((const char *)nv, "nsval") == 0,
        "sic_set_ns_prop + sic_get_ns_prop"
    );
    sic_free(nv);

    /* sic_set_ns_prop — superpone */
    sic_set_ns_prop(radix, ns, SC "attr", SC "nsval2");
    nv = sic_get_ns_prop(radix, SC "attr", SC "http://ns.example");
    affirma(
        nv != NULL && strcmp((const char *)nv, "nsval2") == 0,
        "sic_set_ns_prop superpone"
    );
    sic_free(nv);

    /* attributum sine ns et cum ns eodem nomine non confliguntur */
    sic_set_prop(radix, SC "attr", SC "plain");
    v = sic_get_prop(radix, SC "attr");
    affirma(
        v != NULL && strcmp((const char *)v, "plain") == 0,
        "attr sine ns non confligit"
    );
    sic_free(v);
    nv = sic_get_ns_prop(radix, SC "attr", SC "http://ns.example");
    affirma(
        nv != NULL && strcmp((const char *)nv, "nsval2") == 0,
        "attr cum ns non confligit"
    );
    sic_free(nv);

    sic_free_doc(doc);
}

/* ================================================================
 * VII. entitates et circuitus scriptor/dissector
 * ================================================================ */
static void proba_entitates(void)
{
    printf("  VII. entitates...\n");

    sic_doc_ptr_t doc    = sic_new_doc(SC "1.0");
    sic_node_ptr_t radix = sic_new_node(NULL, SC "r");
    sic_doc_set_root_element(doc, radix);

    /* characteres qui effugiendi sunt */
    sic_set_prop(radix, SC "a", SC "a&b<c\"d'e");
    sic_new_text_child(radix, NULL, SC "t", SC "x&y<z>w");

    sic_save_format_file_enc("artifacta/sic_ent.xml", doc, "UTF-8", 0);
    sic_free_doc(doc);

    /* relege et verifica circuitum */
    doc = sic_read_file("artifacta/sic_ent.xml", NULL, 0);
    affirma(doc != NULL, "relege entitates");
    radix = sic_doc_get_root_element(doc);

    sic_char_t *av = sic_get_prop(radix, SC "a");
    affirma(
        av != NULL && strcmp((const char *)av, "a&b<c\"d'e") == 0,
        "circuitus entitatum in attributo"
    );
    sic_free(av);

    sic_node_ptr_t t = radix->children;
    affirma(
        t != NULL && t->type == SIC_ELEMENT_NODE,
        "elementum t adest"
    );
    sic_char_t *tc = sic_node_get_content(t);
    affirma(
        tc != NULL && strcmp((const char *)tc, "x&y<z>w") == 0,
        "circuitus entitatum in textu"
    );
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
    sic_doc_ptr_t doc    = sic_new_doc(SC "1.0");
    sic_node_ptr_t radix = sic_new_node(NULL, SC "r");
    sic_doc_set_root_element(doc, radix);
    sic_new_child(radix, NULL, SC "a", NULL);
    sic_new_child(radix, NULL, SC "b", NULL);
    sic_save_format_file_enc("artifacta/sic_nb.xml", doc, "UTF-8", 1);
    sic_free_doc(doc);

    /* lege cum NOBLANKS — nulla nodi textus vacui */
    doc = sic_read_file("artifacta/sic_nb.xml", NULL, SIC_PARSE_NOBLANKS);
    radix = sic_doc_get_root_element(doc);
    int textus_vacui = 0;
    for (sic_node_ptr_t c = radix->children; c; c = c->next)
        if (c->type == SIC_TEXT_NODE)
            textus_vacui++;
    affirma(
        textus_vacui == 0,
        "NOBLANKS: nulli nodi textus vacui"
    );

    /* verifica elementa praesentia */
    int elementa = 0;
    for (sic_node_ptr_t c = radix->children; c; c = c->next)
        if (c->type == SIC_ELEMENT_NODE)
            elementa++;
    affirma(elementa == 2, "NOBLANKS: duo elementa praesentia");
    sic_free_doc(doc);

    /* lege sine NOBLANKS — textus vacui adsunt */
    doc = sic_read_file("artifacta/sic_nb.xml", NULL, 0);
    radix = sic_doc_get_root_element(doc);
    textus_vacui = 0;
    for (sic_node_ptr_t c = radix->children; c; c = c->next)
        if (c->type == SIC_TEXT_NODE)
            textus_vacui++;
    affirma(
        textus_vacui > 0,
        "sine NOBLANKS: textus vacui adsunt"
    );
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

    sic_node_ptr_t r2    = sic_new_node(NULL, SC "new");
    sic_node_ptr_t vetus = sic_doc_set_root_element(doc, r2);
    affirma(vetus == r1, "vetus radix reddita");
    affirma(
        sic_doc_get_root_element(doc) == r2,
        "nova radix instituta"
    );

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
    sic_doc_ptr_t doc    = sic_new_doc(SC "1.0");
    sic_node_ptr_t radix = sic_new_node(NULL, SC "Relationships");
    sic_doc_set_root_element(doc, radix);

    sic_ns_ptr_t ns_pkg = sic_new_ns(
        radix,
        SC "http://schemas.example.com/package/relationships", NULL
    );
    sic_set_ns(radix, ns_pkg);

    sic_node_ptr_t rel = sic_new_child(
        radix, NULL,
        SC "Relationship", NULL
    );
    sic_set_prop(rel, SC "Id", SC "rId1");
    sic_set_prop(
        rel, SC "Type",
        SC "http://schemas.example.com/officeDocument"
    );
    sic_set_prop(rel, SC "Target", SC "xl/workbook.xml");

    sic_save_format_file_enc("artifacta/sic_ns.xml", doc, "UTF-8", 1);
    sic_free_doc(doc);

    /* relege et verifica */
    doc   = sic_read_file("artifacta/sic_ns.xml", NULL, SIC_PARSE_NOBLANKS);
    radix = sic_doc_get_root_element(doc);
    affirma(
        sic_strcmp(radix->name, SC "Relationships") == 0,
        "ns: nomen radicis"
    );

    /* search_ns_by_href in filio */
    sic_node_ptr_t ch = radix->children;
    affirma(
        ch != NULL && ch->type == SIC_ELEMENT_NODE,
        "ns: filius adest"
    );

    sic_ns_ptr_t found = sic_search_ns_by_href(
        doc, ch,
        SC "http://schemas.example.com/package/relationships"
    );
    affirma(found != NULL, "search_ns_by_href per filium");

    sic_char_t *tgt = sic_get_prop(ch, SC "Target");
    affirma(
        tgt != NULL &&
        strcmp((const char *)tgt, "xl/workbook.xml") == 0,
        "ns: attributum Target"
    );
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
        "artifacta/sic_non_existens_12345.xml", NULL, 0
    );
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
 * XIII. casus limites NULL — functiones non cadunt
 * ================================================================ */
static void proba_casus_null(void)
{
    printf("  XIII. casus limites NULL...\n");

    /* sic_free_doc(NULL) non cadit */
    sic_free_doc(NULL);
    affirma(1, "sic_free_doc(NULL) non cadit");

    /* sic_free_node(NULL) non cadit */
    sic_free_node(NULL);
    affirma(1, "sic_free_node(NULL) non cadit");

    /* sic_unlink_node(NULL) non cadit */
    sic_unlink_node(NULL);
    affirma(1, "sic_unlink_node(NULL) non cadit");

    /* sic_doc_get_root_element(NULL) */
    affirma(
        sic_doc_get_root_element(NULL) == NULL,
        "sic_doc_get_root_element(NULL)"
    );

    /* sic_doc_set_root_element cum NULL */
    affirma(
        sic_doc_set_root_element(NULL, NULL) == NULL,
        "sic_doc_set_root_element(NULL, NULL)"
    );
    sic_doc_ptr_t doc = sic_new_doc(SC "1.0");
    affirma(
        sic_doc_set_root_element(doc, NULL) == NULL,
        "sic_doc_set_root_element(doc, NULL)"
    );
    sic_free_doc(doc);

    /* sic_add_child cum NULL */
    affirma(
        sic_add_child(NULL, NULL) == NULL,
        "sic_add_child(NULL, NULL)"
    );
    sic_node_ptr_t n = sic_new_node(NULL, SC "x");
    affirma(
        sic_add_child(NULL, n) == NULL,
        "sic_add_child(NULL, n)"
    );
    affirma(
        sic_add_child(n, NULL) == NULL,
        "sic_add_child(n, NULL)"
    );
    sic_free_node(n);

    /* sic_add_prev_sibling cum NULL */
    affirma(
        sic_add_prev_sibling(NULL, NULL) == NULL,
        "sic_add_prev_sibling(NULL, NULL)"
    );

    /* sic_get_prop cum NULL */
    affirma(
        sic_get_prop(NULL, SC "x") == NULL,
        "sic_get_prop(NULL, ...)"
    );
    n = sic_new_node(NULL, SC "x");
    affirma(
        sic_get_prop(n, NULL) == NULL,
        "sic_get_prop(n, NULL)"
    );
    sic_free_node(n);

    /* sic_get_ns_prop cum NULL */
    affirma(
        sic_get_ns_prop(NULL, SC "x", SC "h") == NULL,
        "sic_get_ns_prop(NULL, ...)"
    );

    /* sic_set_prop cum NULL */
    affirma(
        sic_set_prop(NULL, SC "x", SC "v") == NULL,
        "sic_set_prop(NULL, ...)"
    );
    n = sic_new_node(NULL, SC "x");
    affirma(
        sic_set_prop(n, NULL, SC "v") == NULL,
        "sic_set_prop(n, NULL, ...)"
    );
    sic_free_node(n);

    /* sic_set_ns_prop cum NULL */
    affirma(
        sic_set_ns_prop(NULL, NULL, SC "x", SC "v") == NULL,
        "sic_set_ns_prop(NULL, ...)"
    );

    /* sic_unset_prop cum NULL */
    affirma(
        sic_unset_prop(NULL, SC "x") == -1,
        "sic_unset_prop(NULL, ...)"
    );
    n = sic_new_node(NULL, SC "x");
    affirma(
        sic_unset_prop(n, NULL) == -1,
        "sic_unset_prop(n, NULL)"
    );
    sic_free_node(n);

    /* sic_node_get_content(NULL) */
    affirma(
        sic_node_get_content(NULL) == NULL,
        "sic_node_get_content(NULL)"
    );

    /* sic_search_ns_by_href cum NULL href */
    doc = sic_new_doc(SC "1.0");
    sic_node_ptr_t r = sic_new_node(NULL, SC "r");
    sic_doc_set_root_element(doc, r);
    affirma(
        sic_search_ns_by_href(doc, r, NULL) == NULL,
        "sic_search_ns_by_href(NULL href)"
    );
    sic_free_doc(doc);

    /* sic_set_ns cum NULL nodo */
    sic_set_ns(NULL, NULL);
    affirma(1, "sic_set_ns(NULL, NULL) non cadit");

    /* sic_free(NULL) non cadit */
    sic_free(NULL);
    affirma(1, "sic_free(NULL) non cadit");
}

/* ================================================================
 * XIV. sic_new_doc — versio praefinita et explicita
 * ================================================================ */
static void proba_new_doc(void)
{
    printf("  XIV. sic_new_doc...\n");

    /* versio NULL — debet praefinitam "1.0" adhibere */
    sic_doc_ptr_t doc = sic_new_doc(NULL);
    affirma(doc != NULL, "sic_new_doc(NULL)");
    affirma(
        doc->type == SIC_DOCUMENT_NODE,
        "doc->type == DOCUMENT_NODE"
    );
    affirma(
        doc->version != NULL &&
        strcmp((const char *)doc->version, "1.0") == 0,
        "versio praefinita 1.0"
    );
    affirma(
        sic_doc_get_root_element(doc) == NULL,
        "documentum vacuum sine radice"
    );
    sic_free_doc(doc);

    /* versio explicita */
    doc = sic_new_doc(SC "1.1");
    affirma(
        doc->version != NULL &&
        strcmp((const char *)doc->version, "1.1") == 0,
        "versio explicita 1.1"
    );
    sic_free_doc(doc);
}

/* ================================================================
 * XV. sic_new_node et sic_new_text — casus limites
 * ================================================================ */
static void proba_nodi_creatio(void)
{
    printf("  XV. nodi creatio...\n");

    /* nodus cum nomine NULL */
    sic_node_ptr_t n = sic_new_node(NULL, NULL);
    affirma(n != NULL, "sic_new_node(NULL, NULL)");
    affirma(
        n->type == SIC_ELEMENT_NODE,
        "nodus->type == ELEMENT_NODE"
    );
    affirma(n->name == NULL, "nodus->name == NULL");
    sic_free_node(n);

    /* textus cum contentum NULL */
    n = sic_new_text(NULL);
    affirma(n != NULL, "sic_new_text(NULL)");
    affirma(
        n->type == SIC_TEXT_NODE,
        "textus->type == TEXT_NODE"
    );
    affirma(n->content == NULL, "textus->content == NULL");
    sic_free_node(n);

    /* sic_new_child sine parente */
    n = sic_new_child(NULL, NULL, SC "orphanus", SC "val");
    affirma(n != NULL, "sic_new_child sine parente");
    affirma(n->parent == NULL, "orphanus sine parente");
    affirma(n->children != NULL, "orphanus cum textu filio");
    sic_char_t *cont = sic_node_get_content(n);
    affirma(
        cont != NULL &&
        strcmp((const char *)cont, "val") == 0,
        "orphanus contentum"
    );
    sic_free(cont);
    sic_free_node(n);

    /* sic_new_child cum parente, sine contentum */
    sic_node_ptr_t p = sic_new_node(NULL, SC "p");
    n = sic_new_child(p, NULL, SC "c", NULL);
    affirma(n->parent == p, "filius cum parente");
    affirma(n->children == NULL, "filius sine contentum");
    affirma(p->children == n, "parens->children");
    affirma(p->last == n, "parens->last");
    sic_free_node(p);
}

/* ================================================================
 * XVI. CDATA — dissector et contentum
 * ================================================================ */
static void proba_cdata(void)
{
    printf("  XVI. CDATA...\n");

    const char *xml =
        "<?xml version=\"1.0\"?>\n"
        "<r><![CDATA[<non&est>elementum]]></r>";
    FILE *fp = fopen("artifacta/sic_cdata.xml", "w");
    fputs(xml, fp);
    fclose(fp);

    sic_doc_ptr_t doc = sic_read_file("artifacta/sic_cdata.xml", NULL, 0);
    affirma(doc != NULL, "CDATA: lege plicam");
    sic_node_ptr_t radix = sic_doc_get_root_element(doc);
    affirma(radix != NULL, "CDATA: radix");

    sic_char_t *cont = sic_node_get_content(radix);
    affirma(
        cont != NULL &&
        strcmp((const char *)cont, "<non&est>elementum") == 0,
        "CDATA: contentum intactum"
    );
    sic_free(cont);
    sic_free_doc(doc);
}

/* ================================================================
 * XVII. entitates numericae — decimales et hexadecimales
 * ================================================================ */
static void proba_entitates_numericae(void)
{
    printf("  XVII. entitates numericae...\n");

    /* decimalis &#65; = 'A', hexadecimalis &#x42; = 'B' */
    const char *xml =
        "<?xml version=\"1.0\"?>\n"
        "<r>&#65;&#x42;&#169;</r>";
    FILE *fp = fopen("artifacta/sic_entnum.xml", "w");
    fputs(xml, fp);
    fclose(fp);

    sic_doc_ptr_t doc = sic_read_file(
        "artifacta/sic_entnum.xml", NULL, 0
    );
    sic_node_ptr_t radix = sic_doc_get_root_element(doc);
    sic_char_t *cont     = sic_node_get_content(radix);
    affirma(cont != NULL, "entitates numericae: contentum non NULL");

    /* 'A' + 'B' + copyright (UTF-8: 0xC2 0xA9) */
    affirma(cont[0] == 'A', "&#65; == A");
    affirma(cont[1] == 'B', "&#x42; == B");
    /* &#169; = U+00A9 = UTF-8 0xC2 0xA9 */
    affirma(
        (unsigned char)cont[2] == 0xC2 &&
        (unsigned char)cont[3] == 0xA9,
        "&#169; == UTF-8 copyright"
    );
    sic_free(cont);
    sic_free_doc(doc);
}

/* ================================================================
 * XVIII. commentaria XML — praeterita a dissectore
 * ================================================================ */
static void proba_commentaria(void)
{
    printf("  XVIII. commentaria XML...\n");

    const char *xml =
        "<?xml version=\"1.0\"?>\n"
        "<!-- commentarium ante radicem -->\n"
        "<r><!-- commentarium internum --><a/></r>";
    FILE *fp = fopen("artifacta/sic_comm.xml", "w");
    fputs(xml, fp);
    fclose(fp);

    sic_doc_ptr_t doc = sic_read_file(
        "artifacta/sic_comm.xml", NULL, SIC_PARSE_NOBLANKS
    );
    affirma(doc != NULL, "commentaria: lege");
    sic_node_ptr_t radix = sic_doc_get_root_element(doc);
    affirma(radix != NULL, "commentaria: radix");
    affirma(
        sic_strcmp(radix->name, SC "r") == 0,
        "commentaria: nomen radicis"
    );

    /* solum elementum <a/> debet adesse (commentarium praeteritum) */
    int n_elem = 0;
    for (sic_node_ptr_t c = radix->children; c; c = c->next)
        if (c->type == SIC_ELEMENT_NODE)
            n_elem++;
    affirma(n_elem == 1, "commentaria: unum elementum filium");
    sic_free_doc(doc);
}

/* ================================================================
 * XIX. instructiones processandi (PI) — praeteritae
 * ================================================================ */
static void proba_pi(void)
{
    printf("  XIX. instructiones processandi...\n");

    const char *xml =
        "<?xml version=\"1.0\"?>\n"
        "<?target data?>\n"
        "<r><?pi internum?><a/></r>";
    FILE *fp = fopen("artifacta/sic_pi.xml", "w");
    fputs(xml, fp);
    fclose(fp);

    sic_doc_ptr_t doc = sic_read_file(
        "artifacta/sic_pi.xml", NULL, SIC_PARSE_NOBLANKS
    );
    affirma(doc != NULL, "PI: lege");
    sic_node_ptr_t radix = sic_doc_get_root_element(doc);

    int n_elem = 0;
    for (sic_node_ptr_t c = radix->children; c; c = c->next)
        if (c->type == SIC_ELEMENT_NODE)
            n_elem++;
    affirma(n_elem == 1, "PI: unum elementum (PI praeteritae)");
    sic_free_doc(doc);
}

/* ================================================================
 * XX. DOCTYPE — praeterita a dissectore
 * ================================================================ */
static void proba_doctype(void)
{
    printf("  XX. DOCTYPE...\n");

    const char *xml =
        "<?xml version=\"1.0\"?>\n"
        "<!DOCTYPE r [\n"
        "  <!ELEMENT r (#PCDATA)>\n"
        "]>\n"
        "<r>salve</r>";
    FILE *fp = fopen("artifacta/sic_dt.xml", "w");
    fputs(xml, fp);
    fclose(fp);

    sic_doc_ptr_t doc = sic_read_file(
        "artifacta/sic_dt.xml", NULL, 0
    );
    affirma(doc != NULL, "DOCTYPE: lege");
    sic_node_ptr_t radix = sic_doc_get_root_element(doc);
    sic_char_t *cont     = sic_node_get_content(radix);
    affirma(
        cont != NULL &&
        strcmp((const char *)cont, "salve") == 0,
        "DOCTYPE: contentum post DOCTYPE"
    );
    sic_free(cont);
    sic_free_doc(doc);
}

/* ================================================================
 * XXI. BOM UTF-8 — praeterita a dissectore
 * ================================================================ */
static void proba_bom(void)
{
    printf("  XXI. BOM UTF-8...\n");

    FILE *fp = fopen("artifacta/sic_bom.xml", "wb");
    /* BOM UTF-8: EF BB BF */
    fputc(0xEF, fp);
    fputc(0xBB, fp);
    fputc(0xBF, fp);
    fputs("<?xml version=\"1.0\"?><r>bom</r>", fp);
    fclose(fp);

    sic_doc_ptr_t doc = sic_read_file(
        "artifacta/sic_bom.xml", NULL, 0
    );
    affirma(doc != NULL, "BOM: lege");
    sic_node_ptr_t radix = sic_doc_get_root_element(doc);
    sic_char_t *cont     = sic_node_get_content(radix);
    affirma(
        cont != NULL &&
        strcmp((const char *)cont, "bom") == 0,
        "BOM: contentum post BOM"
    );
    sic_free(cont);
    sic_free_doc(doc);
}

/* ================================================================
 * XXII. declaratio XML — versio, codificatio, standalone
 * ================================================================ */
static void proba_declaratio(void)
{
    printf("  XXII. declaratio XML...\n");

    const char *xml =
        "<?xml version=\"1.1\" encoding=\"UTF-8\""
        " standalone=\"yes\"?>\n<r/>";
    FILE *fp = fopen("artifacta/sic_decl.xml", "w");
    fputs(xml, fp);
    fclose(fp);

    sic_doc_ptr_t doc = sic_read_file(
        "artifacta/sic_decl.xml", NULL, 0
    );
    affirma(doc != NULL, "declaratio: lege");
    affirma(
        doc->version != NULL &&
        strcmp((const char *)doc->version, "1.1") == 0,
        "declaratio: versio 1.1"
    );
    affirma(
        doc->encoding != NULL &&
        strcmp((const char *)doc->encoding, "UTF-8") == 0,
        "declaratio: codificatio UTF-8"
    );
    affirma(
        doc->standalone == 1,
        "declaratio: standalone yes"
    );
    sic_free_doc(doc);

    /* sine declaratione — versio praefinita */
    xml = "<r/>";
    fp  = fopen("artifacta/sic_decl2.xml", "w");
    fputs(xml, fp);
    fclose(fp);

    doc = sic_read_file("artifacta/sic_decl2.xml", NULL, 0);
    affirma(doc != NULL, "sine declaratione: lege");
    affirma(
        doc->version != NULL &&
        strcmp((const char *)doc->version, "1.0") == 0,
        "sine declaratione: versio praefinita 1.0"
    );
    sic_free_doc(doc);
}

/* ================================================================
 * XXIII. elementa claudentia se ipsa (<br/>)
 * ================================================================ */
static void proba_self_closing(void)
{
    printf("  XXIII. elementa claudentia se ipsa...\n");

    const char *xml =
        "<?xml version=\"1.0\"?>\n"
        "<r><a/><b x=\"1\"/><c></c></r>";
    FILE *fp = fopen("artifacta/sic_sc.xml", "w");
    fputs(xml, fp);
    fclose(fp);

    sic_doc_ptr_t doc = sic_read_file(
        "artifacta/sic_sc.xml", NULL, SIC_PARSE_NOBLANKS
    );
    sic_node_ptr_t radix = sic_doc_get_root_element(doc);

    int n = 0;
    sic_node_ptr_t b = NULL;
    for (sic_node_ptr_t c = radix->children; c; c = c->next) {
        if (c->type == SIC_ELEMENT_NODE) {
            n++;
            if (sic_strcmp(c->name, SC "b") == 0)
                b = c;
        }
    }
    affirma(n == 3, "self-closing: tria elementa");

    /* <b> habet attributum */
    affirma(b != NULL, "self-closing: inveni b");
    sic_char_t *v = sic_get_prop(b, SC "x");
    affirma(
        v != NULL && strcmp((const char *)v, "1") == 0,
        "self-closing: attributum in b"
    );
    sic_free(v);

    /* <a/> et <c></c> sine filiis */
    sic_node_ptr_t a = radix->children;
    affirma(a->children == NULL, "self-closing: a sine filiis");

    sic_free_doc(doc);
}

/* ================================================================
 * XXIV. arbor profunda — multi gradus nidificati
 * ================================================================ */
static void proba_arbor_profunda(void)
{
    printf("  XXIV. arbor profunda...\n");

    sic_doc_ptr_t doc    = sic_new_doc(SC "1.0");
    sic_node_ptr_t radix = sic_new_node(NULL, SC "r");
    sic_doc_set_root_element(doc, radix);

    /* crea arborem 50 graduum */
    sic_node_ptr_t currens = radix;
    for (int i = 0; i < 50; i++)
        currens = sic_new_child(currens, NULL, SC "n", NULL);
    sic_new_text_child(currens, NULL, SC "folium", SC "profundum");

    /* sic_node_get_content recursivum per omnes gradus */
    sic_char_t *cont = sic_node_get_content(radix);
    affirma(
        cont != NULL &&
        strcmp((const char *)cont, "profundum") == 0,
        "arbor profunda: contentum recursivum"
    );
    sic_free(cont);

    /* scribe et relege */
    sic_save_format_file_enc(
        "artifacta/sic_prof.xml", doc, "UTF-8", 1
    );
    sic_free_doc(doc);

    doc = sic_read_file(
        "artifacta/sic_prof.xml", NULL, SIC_PARSE_NOBLANKS
    );
    radix = sic_doc_get_root_element(doc);
    cont  = sic_node_get_content(radix);
    affirma(
        cont != NULL &&
        strcmp((const char *)cont, "profundum") == 0,
        "arbor profunda: circuitus scriptor/dissector"
    );
    sic_free(cont);
    sic_free_doc(doc);
}

/* ================================================================
 * XXV. contentum mixtum — textus et elementa intermixta
 * ================================================================ */
static void proba_contentum_mixtum(void)
{
    printf("  XXV. contentum mixtum...\n");

    const char *xml =
        "<?xml version=\"1.0\"?>\n"
        "<r>ante<m/>inter<n/>post</r>";
    FILE *fp = fopen("artifacta/sic_mix.xml", "w");
    fputs(xml, fp);
    fclose(fp);

    sic_doc_ptr_t doc = sic_read_file(
        "artifacta/sic_mix.xml", NULL, 0
    );
    sic_node_ptr_t radix = sic_doc_get_root_element(doc);

    /* contentum totum concatenatum */
    sic_char_t *cont = sic_node_get_content(radix);
    affirma(
        cont != NULL &&
        strcmp((const char *)cont, "anteinterpost") == 0,
        "mixtum: contentum concatenatum"
    );
    sic_free(cont);

    /* numera nodos */
    int n_text = 0, n_elem = 0;
    for (sic_node_ptr_t c = radix->children; c; c = c->next) {
        if (c->type == SIC_TEXT_NODE)
            n_text++;
        if (c->type == SIC_ELEMENT_NODE)
            n_elem++;
    }
    affirma(n_text == 3, "mixtum: tres nodi textus");
    affirma(n_elem == 2, "mixtum: duo elementa");

    sic_free_doc(doc);
}

/* ================================================================
 * XXVI. sic_save_format_fp — scribe in FILE*
 * ================================================================ */
static void proba_save_fp(void)
{
    printf("  XXVI. sic_save_format_fp...\n");

    sic_doc_ptr_t doc    = sic_new_doc(SC "1.0");
    sic_node_ptr_t radix = sic_new_node(NULL, SC "r");
    sic_doc_set_root_element(doc, radix);
    sic_new_text_child(radix, NULL, SC "a", SC "val");

    FILE *fp = fopen("artifacta/sic_fp.xml", "w");
    sic_save_format_fp(fp, doc, "UTF-8", 0);
    fclose(fp);
    sic_free_doc(doc);

    /* relege et verifica */
    doc = sic_read_file(
        "artifacta/sic_fp.xml", NULL, SIC_PARSE_NOBLANKS
    );
    affirma(doc != NULL, "save_fp: lege");
    radix = sic_doc_get_root_element(doc);
    affirma(
        sic_strcmp(radix->name, SC "r") == 0,
        "save_fp: nomen radicis"
    );
    sic_node_ptr_t a = radix->children;
    affirma(
        a != NULL && sic_strcmp(a->name, SC "a") == 0,
        "save_fp: filius a"
    );
    sic_char_t *cont = sic_node_get_content(a);
    affirma(
        cont != NULL &&
        strcmp((const char *)cont, "val") == 0,
        "save_fp: contentum filii"
    );
    sic_free(cont);
    sic_free_doc(doc);

    /* sic_save_format_fp cum NULL non cadit */
    sic_save_format_fp(NULL, NULL, NULL, 0);
    affirma(1, "save_fp(NULL, NULL) non cadit");

    /* sic_save_format_file_enc cum NULL via */
    affirma(
        sic_save_format_file_enc(NULL, NULL, NULL, 0) == -1,
        "save_file_enc(NULL) reddit -1"
    );
}

/* ================================================================
 * XXVII. spatia nominum — dissectio praefixorum ex plica
 * ================================================================ */
static void proba_ns_dissectio(void)
{
    printf("  XXVII. spatia nominum dissectio...\n");

    const char *xml =
        "<?xml version=\"1.0\"?>\n"
        "<root xmlns=\"http://default.ns\""
        " xmlns:p=\"http://prefix.ns\">\n"
        "  <p:child p:attr=\"pval\" plain=\"pla\"/>\n"
        "  <defchild/>\n"
        "</root>";
    FILE *fp = fopen("artifacta/sic_nsd.xml", "w");
    fputs(xml, fp);
    fclose(fp);

    sic_doc_ptr_t doc = sic_read_file(
        "artifacta/sic_nsd.xml", NULL, SIC_PARSE_NOBLANKS
    );
    sic_node_ptr_t radix = sic_doc_get_root_element(doc);
    affirma(
        sic_strcmp(radix->name, SC "root") == 0,
        "ns_dissectio: nomen radicis"
    );

    /* radix habet ns praefinitum */
    affirma(radix->ns != NULL, "ns_dissectio: radix habet ns");
    affirma(
        radix->ns->prefix == NULL,
        "ns_dissectio: ns praefinitum sine praefixo"
    );

    /* primus filius: p:child */
    sic_node_ptr_t ch = radix->children;
    affirma(ch != NULL, "ns_dissectio: primus filius");
    affirma(
        sic_strcmp(ch->name, SC "child") == 0,
        "ns_dissectio: nomen locale 'child'"
    );
    affirma(
        ch->ns != NULL && ch->ns->prefix != NULL &&
        strcmp((const char *)ch->ns->prefix, "p") == 0,
        "ns_dissectio: praefixum p"
    );

    /* p:attr attributum cum ns */
    sic_char_t *pv = sic_get_ns_prop(
        ch, SC "attr",
        SC "http://prefix.ns"
    );
    affirma(
        pv != NULL &&
        strcmp((const char *)pv, "pval") == 0,
        "ns_dissectio: p:attr"
    );
    sic_free(pv);

    /* plain attributum sine ns */
    sic_char_t *pl = sic_get_prop(ch, SC "plain");
    affirma(
        pl != NULL &&
        strcmp((const char *)pl, "pla") == 0,
        "ns_dissectio: plain attr"
    );
    sic_free(pl);

    /* secundus filius: defchild cum ns praefinito */
    sic_node_ptr_t ch2 = ch->next;
    affirma(
        ch2 != NULL &&
        sic_strcmp(ch2->name, SC "defchild") == 0,
        "ns_dissectio: defchild"
    );
    affirma(
        ch2->ns != NULL &&
        ch2->ns->prefix == NULL,
        "ns_dissectio: defchild ns praefinitum"
    );

    sic_free_doc(doc);
}

/* ================================================================
 * XXVIII. circuitus formatus et non-formatus
 * ================================================================ */
static void proba_formatio(void)
{
    printf("  XXVIII. formatio...\n");

    sic_doc_ptr_t doc    = sic_new_doc(SC "1.0");
    sic_node_ptr_t radix = sic_new_node(NULL, SC "r");
    sic_doc_set_root_element(doc, radix);
    sic_new_text_child(radix, NULL, SC "a", SC "1");
    sic_new_text_child(radix, NULL, SC "b", SC "2");

    /* scribe formatum (forma=1) */
    sic_save_format_file_enc(
        "artifacta/sic_fmt1.xml", doc, "UTF-8", 1
    );
    /* scribe non-formatum (forma=0) */
    sic_save_format_file_enc(
        "artifacta/sic_fmt0.xml", doc, "UTF-8", 0
    );
    sic_free_doc(doc);

    /* relege ambo et verifica contentum identicum */
    doc = sic_read_file(
        "artifacta/sic_fmt1.xml", NULL, SIC_PARSE_NOBLANKS
    );
    sic_node_ptr_t r1 = sic_doc_get_root_element(doc);
    sic_char_t *c1    = sic_node_get_content(r1);
    sic_free_doc(doc);

    doc = sic_read_file(
        "artifacta/sic_fmt0.xml", NULL, SIC_PARSE_NOBLANKS
    );
    sic_node_ptr_t r0 = sic_doc_get_root_element(doc);
    sic_char_t *c0    = sic_node_get_content(r0);
    sic_free_doc(doc);

    affirma(
        c1 != NULL && c0 != NULL &&
        strcmp((const char *)c1, (const char *)c0) == 0,
        "formatio: contentum identicum"
    );
    affirma(
        strcmp((const char *)c1, "12") == 0,
        "formatio: contentum rectum"
    );
    sic_free(c1);
    sic_free(c0);

    /* plica formata longior est (continet '\n' et spatia) */
    FILE *fp1 = fopen("artifacta/sic_fmt1.xml", "rb");
    FILE *fp0 = fopen("artifacta/sic_fmt0.xml", "rb");
    fseek(fp1, 0, SEEK_END);
    fseek(fp0, 0, SEEK_END);
    long mag1 = ftell(fp1);
    long mag0 = ftell(fp0);
    fclose(fp1);
    fclose(fp0);
    affirma(
        mag1 > mag0,
        "formatio: plica formata longior"
    );
}

/* ================================================================
 * XXIX. sic_node_get_content — nodus vacuus
 * ================================================================ */
static void proba_contentum_vacuum(void)
{
    printf("  XXIX. contentum vacuum...\n");

    /* elementum sine filiis — reddit chordam vacuam */
    sic_node_ptr_t n = sic_new_node(NULL, SC "vacuus");
    sic_char_t *cont = sic_node_get_content(n);
    affirma(cont != NULL, "vacuum: non NULL");
    affirma(
        strcmp((const char *)cont, "") == 0,
        "vacuum: chorda vacua"
    );
    sic_free(cont);
    sic_free_node(n);
}

/* ================================================================
 * XXX. sic_new_ns sine nodo
 * ================================================================ */
static void proba_ns_sine_nodo(void)
{
    printf("  XXX. sic_new_ns sine nodo...\n");

    sic_ns_ptr_t ns = sic_new_ns(
        NULL, SC "http://test.ns",
        SC "t"
    );
    affirma(ns != NULL, "ns sine nodo: creatum");
    affirma(
        ns->href != NULL &&
        strcmp((const char *)ns->href, "http://test.ns") == 0,
        "ns sine nodo: href"
    );
    affirma(
        ns->prefix != NULL &&
        strcmp((const char *)ns->prefix, "t") == 0,
        "ns sine nodo: praefixum"
    );
    affirma(ns->next == NULL, "ns sine nodo: next == NULL");

    /* libera manu (non in arbore) */
    free((void *)ns->href);
    free((void *)ns->prefix);
    free(ns);
}

/* ================================================================
 * XXXI. unlink et readd — nodus movetur inter parentes
 * ================================================================ */
static void proba_unlink_readd(void)
{
    printf("  XXXI. unlink et readd...\n");

    sic_doc_ptr_t doc    = sic_new_doc(SC "1.0");
    sic_node_ptr_t radix = sic_new_node(NULL, SC "r");
    sic_doc_set_root_element(doc, radix);

    sic_node_ptr_t p1 = sic_new_child(radix, NULL, SC "p1", NULL);
    sic_node_ptr_t p2 = sic_new_child(radix, NULL, SC "p2", NULL);
    sic_node_ptr_t ch = sic_new_child(p1, NULL, SC "ch", NULL);

    affirma(ch->parent == p1, "readd: initio in p1");
    affirma(p1->children == ch, "readd: p1->children == ch");

    /* move ch ad p2 */
    sic_add_child(p2, ch);
    affirma(ch->parent == p2, "readd: post move in p2");
    affirma(p1->children == NULL, "readd: p1 vacuus");
    affirma(p2->children == ch, "readd: p2->children == ch");

    sic_free_doc(doc);
}

/* ================================================================
 * XXXII. unlink primum et ultimum filium
 * ================================================================ */
static void proba_unlink_extrema(void)
{
    printf("  XXXII. unlink extrema...\n");

    sic_doc_ptr_t doc = sic_new_doc(SC "1.0");
    sic_node_ptr_t r  = sic_new_node(NULL, SC "r");
    sic_doc_set_root_element(doc, r);

    sic_node_ptr_t a = sic_new_child(r, NULL, SC "a", NULL);
    sic_node_ptr_t b = sic_new_child(r, NULL, SC "b", NULL);
    sic_node_ptr_t c = sic_new_child(r, NULL, SC "c", NULL);

    /* unlink primum */
    sic_unlink_node(a);
    affirma(r->children == b, "unlink primus: children == b");
    affirma(b->prev == NULL, "unlink primus: b->prev == NULL");
    sic_free_node(a);

    /* unlink ultimum */
    sic_unlink_node(c);
    affirma(r->last == b, "unlink ultimus: last == b");
    affirma(b->next == NULL, "unlink ultimus: b->next == NULL");
    sic_free_node(c);

    /* unlink unicum filium */
    sic_unlink_node(b);
    affirma(r->children == NULL, "unlink unicus: children NULL");
    affirma(r->last == NULL, "unlink unicus: last NULL");
    sic_free_node(b);

    sic_free_doc(doc);
}

/* ================================================================
 * XXXIII. plures proprietates — ordo servatur
 * ================================================================ */
static void proba_proprietates_ordo(void)
{
    printf("  XXXIII. proprietates ordo...\n");

    sic_doc_ptr_t doc = sic_new_doc(SC "1.0");
    sic_node_ptr_t r  = sic_new_node(NULL, SC "r");
    sic_doc_set_root_element(doc, r);

    sic_set_prop(r, SC "alpha", SC "1");
    sic_set_prop(r, SC "beta", SC "2");
    sic_set_prop(r, SC "gamma", SC "3");

    /* scribe et relege */
    sic_save_format_file_enc(
        "artifacta/sic_ord.xml", doc, "UTF-8", 0
    );
    sic_free_doc(doc);

    doc = sic_read_file("artifacta/sic_ord.xml", NULL, 0);
    sic_node_ptr_t r2 = sic_doc_get_root_element(doc);

    /* verifica omnes proprietates adsunt */
    sic_char_t *va = sic_get_prop(r2, SC "alpha");
    sic_char_t *vb = sic_get_prop(r2, SC "beta");
    sic_char_t *vg = sic_get_prop(r2, SC "gamma");
    affirma(
        va && strcmp((const char *)va, "1") == 0,
        "ordo: alpha"
    );
    affirma(
        vb && strcmp((const char *)vb, "2") == 0,
        "ordo: beta"
    );
    affirma(
        vg && strcmp((const char *)vg, "3") == 0,
        "ordo: gamma"
    );
    sic_free(va);
    sic_free(vb);
    sic_free(vg);

    /* unset medium, cetera persistunt */
    sic_unset_prop(r2, SC "beta");
    affirma(
        sic_get_prop(r2, SC "beta") == NULL,
        "ordo: beta amota"
    );
    va = sic_get_prop(r2, SC "alpha");
    vg = sic_get_prop(r2, SC "gamma");
    affirma(
        va && strcmp((const char *)va, "1") == 0,
        "ordo: alpha post unset beta"
    );
    affirma(
        vg && strcmp((const char *)vg, "3") == 0,
        "ordo: gamma post unset beta"
    );
    sic_free(va);
    sic_free(vg);

    sic_free_doc(doc);
}

/* ================================================================
 * XXXIV. attributum cum valore NULL
 * ================================================================ */
static void proba_attr_valor_null(void)
{
    printf("  XXXIV. attributum valor NULL...\n");

    sic_node_ptr_t n = sic_new_node(NULL, SC "n");
    sic_set_prop(n, SC "nul", NULL);

    /* sic_get_prop reddit NULL si valor est NULL */
    sic_char_t *v = sic_get_prop(n, SC "nul");
    affirma(v == NULL, "attr NULL: reddit NULL");

    /* superpone cum valore */
    sic_set_prop(n, SC "nul", SC "nunc");
    v = sic_get_prop(n, SC "nul");
    affirma(
        v != NULL && strcmp((const char *)v, "nunc") == 0,
        "attr NULL: superposita cum valore"
    );
    sic_free(v);

    sic_free_node(n);
}

/* ================================================================
 * XXXV. apices singuli in attributis
 * ================================================================ */
static void proba_apices_singuli(void)
{
    printf("  XXXV. apices singuli...\n");

    const char *xml =
        "<?xml version=\"1.0\"?>\n"
        "<r a='singuli' b=\"duplices\"/>";
    FILE *fp = fopen("artifacta/sic_ap.xml", "w");
    fputs(xml, fp);
    fclose(fp);

    sic_doc_ptr_t doc = sic_read_file(
        "artifacta/sic_ap.xml", NULL, 0
    );
    sic_node_ptr_t radix = sic_doc_get_root_element(doc);

    sic_char_t *va = sic_get_prop(radix, SC "a");
    sic_char_t *vb = sic_get_prop(radix, SC "b");
    affirma(
        va != NULL &&
        strcmp((const char *)va, "singuli") == 0,
        "apices singuli: attr a"
    );
    affirma(
        vb != NULL &&
        strcmp((const char *)vb, "duplices") == 0,
        "apices singuli: attr b"
    );
    sic_free(va);
    sic_free(vb);
    sic_free_doc(doc);
}

/* ================================================================
 * XXXVI. scriptor — spatia nominum in exitu
 * ================================================================ */
static void proba_scriptor_ns(void)
{
    printf("  XXXVI. scriptor spatia nominum...\n");

    sic_doc_ptr_t doc = sic_new_doc(SC "1.0");
    sic_node_ptr_t r  = sic_new_node(NULL, SC "r");
    sic_doc_set_root_element(doc, r);

    sic_ns_ptr_t ns = sic_new_ns(
        r,
        SC "http://test.example", SC "t"
    );
    sic_set_ns(r, ns);
    sic_new_child(r, NULL, SC "plain", NULL);

    sic_save_format_file_enc(
        "artifacta/sic_wns.xml", doc, "UTF-8", 0
    );
    sic_free_doc(doc);

    /* relege et verifica praefixum in radice */
    doc = sic_read_file(
        "artifacta/sic_wns.xml", NULL, SIC_PARSE_NOBLANKS
    );
    sic_node_ptr_t r2 = sic_doc_get_root_element(doc);
    affirma(r2->ns != NULL, "scriptor_ns: radix habet ns");
    affirma(
        r2->ns->prefix != NULL &&
        strcmp((const char *)r2->ns->prefix, "t") == 0,
        "scriptor_ns: praefixum t"
    );

    sic_ns_ptr_t f = sic_search_ns_by_href(
        doc, r2,
        SC "http://test.example"
    );
    affirma(f != NULL, "scriptor_ns: search_ns_by_href");
    sic_free_doc(doc);
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

    /* probationes novae comprehensivae */
    proba_casus_null();
    proba_new_doc();
    proba_nodi_creatio();
    proba_cdata();
    proba_entitates_numericae();
    proba_commentaria();
    proba_pi();
    proba_doctype();
    proba_bom();
    proba_declaratio();
    proba_self_closing();
    proba_arbor_profunda();
    proba_contentum_mixtum();
    proba_save_fp();
    proba_ns_dissectio();
    proba_formatio();
    proba_contentum_vacuum();
    proba_ns_sine_nodo();
    proba_unlink_readd();
    proba_unlink_extrema();
    proba_proprietates_ordo();
    proba_attr_valor_null();
    proba_apices_singuli();
    proba_scriptor_ns();

#if 0
    /* purga plicas temporales */
    remove("artifacta/sic_proba1.xml");
    remove("artifacta/sic_ent.xml");
    remove("artifacta/sic_nb.xml");
    remove("artifacta/sic_ns.xml");
    remove("artifacta/sic_cdata.xml");
    remove("artifacta/sic_entnum.xml");
    remove("artifacta/sic_comm.xml");
    remove("artifacta/sic_pi.xml");
    remove("artifacta/sic_dt.xml");
    remove("artifacta/sic_bom.xml");
    remove("artifacta/sic_decl.xml");
    remove("artifacta/sic_decl2.xml");
    remove("artifacta/sic_sc.xml");
    remove("artifacta/sic_prof.xml");
    remove("artifacta/sic_mix.xml");
    remove("artifacta/sic_fp.xml");
    remove("artifacta/sic_nsd.xml");
    remove("artifacta/sic_fmt1.xml");
    remove("artifacta/sic_fmt0.xml");
    remove("artifacta/sic_ord.xml");
    remove("artifacta/sic_ap.xml");
    remove("artifacta/sic_wns.xml");
#endif

    printf(
        "\n%d/%d probationes transierunt.\n",
        num_rectae, num_probationes
    );
    return (num_rectae == num_probationes) ? 0 : 1;
}
