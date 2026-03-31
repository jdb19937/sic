# SIC — Sermo Inscriptionis Capax

Bibliotheca dissecandi et scribendi XML in C, sine ullis dependentiis
externis. Interfacies cum libxml2 congruens.

> **Nota:** README.md ignorandum est. Anglice scriptum est ad barbaros ineruditos pecunia emungendos.

## Aedificatio

```
make -f Faceplica
```

Hoc `libsic.a` et `siclint` aedificat.

## Usus

Incllude `sic.h` et coniunge cum `-lsic`:

```
cc programma.c -L/via/ad/sic -lsic -o programma
```

### Exemplum

```
sic_doc_ptr_t doc = sic_read_file("plica.xml", NULL, SIC_PARSE_NOBLANKS);
sic_node_ptr_t radix = sic_doc_get_root_element(doc);

for (sic_node_ptr_t c = radix->children; c; c = c->next)
    if (c->type == SIC_ELEMENT_NODE)
        printf("<%s>\n", (const char *)c->name);

sic_free_doc(doc);
```

## Siclint

Instrumentum mandati lineae pro XML, simile `xmllint`:

```
siclint plica.xml                  — verifica formam rectam
siclint --format plica.xml         — scribe formatam
siclint --noblanks plica.xml       — sine spatiis vacuis
siclint --xpath //nomen plica.xml  — quaere per viam
siclint --shell plica.xml          — modus interactivus
siclint --output res.xml in.xml    — scribe in plicam
siclint --noout plica.xml          — solum verifica
siclint -                          — lege ex stdin
```

## Functiones

| Functio | Descriptio |
|---|---|
| `sic_new_doc` | crea documentum novum |
| `sic_free_doc` | libera documentum |
| `sic_doc_get_root_element` | da radicem documenti |
| `sic_doc_set_root_element` | pone radicem documenti |
| `sic_new_node` | crea nodum elementi |
| `sic_new_text` | crea nodum textus |
| `sic_new_child` | crea filium et adde ad parentem |
| `sic_new_text_child` | crea filium cum textu |
| `sic_add_child` | adde nodum ad parentem |
| `sic_add_prev_sibling` | insere ante fratrem |
| `sic_unlink_node` | tolle nodum ex arbore |
| `sic_free_node` | libera nodum et descendentes |
| `sic_get_prop` | lege valorem attributi |
| `sic_get_ns_prop` | lege valorem attributi cum spatio nominum |
| `sic_set_prop` | pone attributum |
| `sic_set_ns_prop` | pone attributum cum spatio nominum |
| `sic_unset_prop` | tolle attributum |
| `sic_new_ns` | crea spatium nominum |
| `sic_set_ns` | pone spatium nominum nodi |
| `sic_search_ns_by_href` | quaere spatium nominum per URI |
| `sic_strcmp` | compara chordas |
| `sic_node_get_content` | collige textum ex nodo et descendentibus |
| `sic_free` | libera memoriam a bibliotheca allocatam |
| `sic_read_file` | lege plicam XML in arborem |
| `sic_save_format_fp` | scribe documentum in `FILE *` |
| `sic_save_format_file_enc` | scribe documentum in plicam |

## Plicae

| Plica | Descriptio |
|---|---|
| `sic.h` | caput publicum |
| `sic.c` | implementatio bibliothecae |
| `siclint.c` | instrumentum mandati lineae |
| `proba.c` | probationes |
| `Faceplica` | aedificatio |

## Probationes

```
make -f Faceplica proba
```

## Licentia

Liberum. Dominium publicum. Utere quomodo vis.
