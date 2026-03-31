/*
 * sic.h — Sermo Inscriptionis Capax
 *
 * Bibliotheca dissecandi et scribendi XML, cum libxml2 congruens.
 * Praefixum sic_ pro xml, typorum suffixum _t.
 */
#ifndef SIC_H
#define SIC_H

#include <stdio.h>

/* ================================================================
 * typi fundamentales
 * ================================================================ */

typedef unsigned char sic_char_t;

typedef enum {
    SIC_ELEMENT_NODE       = 1,
    SIC_ATTRIBUTE_NODE     = 2,
    SIC_TEXT_NODE          = 3,
    SIC_CDATA_SECTION_NODE = 4,
    SIC_COMMENT_NODE       = 8,
    SIC_DOCUMENT_NODE      = 9
} sic_element_type_t;

/* vexilla dissectoris */
#define SIC_PARSE_NONET    (1 << 0)
#define SIC_PARSE_NOBLANKS (1 << 1)
#define SIC_PARSE_RECOVER  (1 << 2)
#define SIC_PARSE_NOERROR  (1 << 3)

/* ================================================================
 * declarationes praemissae et typi indicatorum
 * ================================================================ */

typedef struct sic_ns   sic_ns_t;
typedef struct sic_attr sic_attr_t;
typedef struct sic_node sic_node_t;
typedef struct sic_doc  sic_doc_t;

typedef sic_doc_t   *sic_doc_ptr_t;
typedef sic_node_t  *sic_node_ptr_t;
typedef sic_ns_t    *sic_ns_ptr_t;
typedef sic_attr_t  *sic_attr_ptr_t;

/* ================================================================
 * structurae
 * ================================================================ */

struct sic_ns {
    sic_ns_ptr_t       next;      /* proxima declaratio in eodem nodo */
    const sic_char_t  *href;      /* URI spatii nominum */
    const sic_char_t  *prefix;    /* praefixum (NULL = spatium praefinitum) */
};

struct sic_attr {
    sic_element_type_t type;      /* SIC_ATTRIBUTE_NODE */
    const sic_char_t  *name;      /* nomen locale */
    sic_char_t        *value;     /* valor attributi */
    sic_attr_ptr_t     next;
    sic_attr_ptr_t     prev;
    sic_node_ptr_t     parent;
    sic_doc_ptr_t      doc;
    sic_ns_ptr_t       ns;
};

struct sic_node {
    sic_element_type_t type;
    const sic_char_t  *name;      /* nomen elementi */
    sic_node_ptr_t     children;  /* primus filius */
    sic_node_ptr_t     last;      /* ultimus filius */
    sic_node_ptr_t     parent;
    sic_node_ptr_t     next;      /* proximus frater */
    sic_node_ptr_t     prev;      /* frater prior */
    sic_doc_ptr_t      doc;
    sic_char_t        *content;   /* textus (nodi textus/CDATA) */
    sic_attr_ptr_t     properties;/* attributa */
    sic_ns_ptr_t       ns;        /* spatium nominum elementi */
    sic_ns_ptr_t       nsDef;     /* declarationes spatiorum nominum */
};

struct sic_doc {
    sic_element_type_t type;      /* SIC_DOCUMENT_NODE */
    sic_node_ptr_t     children;
    sic_node_ptr_t     last;
    sic_char_t        *version;
    sic_char_t        *encoding;
    int                standalone;
};

/* ================================================================
 * functiones — documenta
 * ================================================================ */

sic_doc_ptr_t  sic_new_doc(const sic_char_t *versio);
void           sic_free_doc(sic_doc_ptr_t doc);
sic_node_ptr_t sic_doc_get_root_element(sic_doc_ptr_t doc);
sic_node_ptr_t sic_doc_set_root_element(sic_doc_ptr_t doc,
                                        sic_node_ptr_t radix);

/* ================================================================
 * functiones — nodi
 * ================================================================ */

sic_node_ptr_t sic_new_node(sic_ns_ptr_t ns, const sic_char_t *nomen);
sic_node_ptr_t sic_new_text(const sic_char_t *contentum);
sic_node_ptr_t sic_new_child(sic_node_ptr_t parens, sic_ns_ptr_t ns,
                             const sic_char_t *nomen,
                             const sic_char_t *contentum);
sic_node_ptr_t sic_new_text_child(sic_node_ptr_t parens, sic_ns_ptr_t ns,
                                  const sic_char_t *nomen,
                                  const sic_char_t *contentum);
sic_node_ptr_t sic_add_child(sic_node_ptr_t parens,
                             sic_node_ptr_t filius);
sic_node_ptr_t sic_add_prev_sibling(sic_node_ptr_t frater,
                                    sic_node_ptr_t novus);
void           sic_unlink_node(sic_node_ptr_t nodus);
void           sic_free_node(sic_node_ptr_t nodus);

/* ================================================================
 * functiones — proprietates
 * ================================================================ */

sic_char_t    *sic_get_prop(sic_node_ptr_t nodus,
                            const sic_char_t *nomen);
sic_char_t    *sic_get_ns_prop(sic_node_ptr_t nodus,
                               const sic_char_t *nomen,
                               const sic_char_t *href);
sic_attr_ptr_t sic_set_prop(sic_node_ptr_t nodus,
                            const sic_char_t *nomen,
                            const sic_char_t *valor);
sic_attr_ptr_t sic_set_ns_prop(sic_node_ptr_t nodus, sic_ns_ptr_t ns,
                               const sic_char_t *nomen,
                               const sic_char_t *valor);
int            sic_unset_prop(sic_node_ptr_t nodus,
                              const sic_char_t *nomen);

/* ================================================================
 * functiones — spatia nominum
 * ================================================================ */

sic_ns_ptr_t sic_new_ns(sic_node_ptr_t nodus,
                        const sic_char_t *href,
                        const sic_char_t *praefixum);
void         sic_set_ns(sic_node_ptr_t nodus, sic_ns_ptr_t ns);
sic_ns_ptr_t sic_search_ns_by_href(sic_doc_ptr_t doc,
                                   sic_node_ptr_t nodus,
                                   const sic_char_t *href);

/* ================================================================
 * functiones — chordae et memoria
 * ================================================================ */

int         sic_strcmp(const sic_char_t *s1, const sic_char_t *s2);
sic_char_t *sic_node_get_content(sic_node_ptr_t nodus);
void        sic_free(void *ptr);

/* ================================================================
 * functiones — legere et scribere
 * ================================================================ */

sic_doc_ptr_t sic_read_file(const char *via,
                            const char *codificatio,
                            int optiones);

void sic_save_format_fp(FILE *fp, sic_doc_ptr_t doc,
                        const char *codificatio, int forma);

int sic_save_format_file_enc(const char *via, sic_doc_ptr_t doc,
                             const char *codificatio, int forma);

#endif /* SIC_H */
