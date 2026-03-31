# SIC — Sermo Inscriptionis Capax

A complete, self-contained XML parser and DOM implementation in pure C. No libxml2. No expat. No dependencies whatsoever — just a C compiler and `make`. SIC reads XML, builds a full DOM tree, manipulates it however you need, and writes it back out. The entire implementation fits in a single `.c` file.

SIC provides a libxml2-compatible API surface, which means you can drop it into projects that expect libxml2's data structures — the same tree traversal patterns, the same property accessors, the same namespace-aware operations — without dragging in a quarter-million lines of external code and a maze of `./configure` flags.

## What You Get

Everything you need for serious XML work:

- **Full DOM tree** — elements, text nodes, CDATA sections, attributes, namespaces, all properly linked with parent/child/sibling pointers
- **Namespace-aware parsing** — default namespaces, prefixed namespaces, namespace-qualified attributes, hierarchical namespace resolution by walking up the tree
- **Complete round-trip fidelity** — parse XML, modify the tree, write it back. Entity encoding, namespace declarations, attribute ordering — all preserved
- **UTF-8 throughout** — BOM detection, numeric character references (decimal and hex) decoded to proper UTF-8 sequences
- **Zero allocator overhead** — every node, every attribute, every string is a simple `malloc`/`free`. No custom allocators, no arena pools, no garbage collectors

## The API

SIC mirrors the libxml2 patterns that thousands of C programs already use:

```c
/* read and traverse */
sic_doc_ptr_t doc = sic_read_file("data.xml", NULL, SIC_PARSE_NOBLANKS);
sic_node_ptr_t root = sic_doc_get_root_element(doc);

for (sic_node_ptr_t child = root->children; child; child = child->next) {
    if (child->type == SIC_ELEMENT_NODE) {
        sic_char_t *val = sic_get_prop(child, (const sic_char_t *)"id");
        /* ... */
        sic_free(val);
    }
}

/* build from scratch */
sic_doc_ptr_t doc = sic_new_doc((const sic_char_t *)"1.0");
sic_node_ptr_t root = sic_new_node(NULL, (const sic_char_t *)"catalog");
sic_doc_set_root_element(doc, root);
sic_new_text_child(root, NULL, (const sic_char_t *)"item",
                   (const sic_char_t *)"widget");
sic_save_format_file_enc("output.xml", doc, "UTF-8", 1);

sic_free_doc(doc);
```

Namespaces work exactly as you'd expect:

```c
sic_ns_ptr_t ns = sic_new_ns(root,
    (const sic_char_t *)"http://example.com/schema", (const sic_char_t *)"ex");
sic_set_ns(root, ns);
sic_set_ns_prop(root, ns, (const sic_char_t *)"attr",
                (const sic_char_t *)"value");

/* search by href, walking up the tree */
sic_ns_ptr_t found = sic_search_ns_by_href(doc, some_deep_node,
    (const sic_char_t *)"http://example.com/schema");
```

## Siclint

SIC ships with `siclint`, a command-line XML tool in the spirit of `xmllint`:

```bash
siclint document.xml              # validate well-formedness
siclint --format document.xml     # pretty-print with indentation
siclint --xpath "//item" data.xml # query elements by path
siclint --shell data.xml          # interactive exploration mode
siclint --census data.xml         # count elements, text nodes, attributes
siclint - < stream.xml            # read from stdin
```

The interactive shell lets you navigate the DOM tree, inspect attributes, run XPath queries, and save modifications — all without writing a single line of code.

## Building

```bash
make -f Faceplica        # builds libsic.a and siclint
make -f Faceplica proba  # run the test suite
make -f Faceplica purga  # clean
```

No `./configure`. No CMake. No pkg-config. Just `make`.

## License

Free. Public domain. Use however you like.
