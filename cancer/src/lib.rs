/*
 * sic — Sermo Inscriptionis Capax
 *
 * Translatio fidelis Rustica ex bibliotheca C.
 * Dissector XML, arbor DOM, scriptor.
 * Sine ullis dependentiis externis.
 */

use std::fmt;
use std::fs;
use std::io::{self, Write};

/* ================================================================
 * typi fundamentales
 * ================================================================ */

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(i32)]
pub enum SicElementType {
    ElementNode      = 1,
    AttributeNode    = 2,
    TextNode         = 3,
    CdataSectionNode = 4,
    CommentNode      = 8,
    DocumentNode     = 9,
}

/* vexilla dissectoris */
pub const SIC_PARSE_NONET: i32    = 1 << 0;
pub const SIC_PARSE_NOBLANKS: i32 = 1 << 1;
pub const SIC_PARSE_RECOVER: i32  = 1 << 2;
pub const SIC_PARSE_NOERROR: i32  = 1 << 3;

/* ================================================================
 * structurae
 * ================================================================ */

#[derive(Debug, Clone)]
pub struct SicNs {
    pub href:   Option<String>,
    pub prefix: Option<String>,
}

#[derive(Debug, Clone)]
pub struct SicAttr {
    pub nomen: String,
    pub valor: Option<String>,
    pub ns:    Option<usize>, /* index in nodo.ns_defs */
}

#[derive(Debug)]
pub struct SicNode {
    pub genus:        SicElementType,
    pub nomen:        Option<String>,
    pub contentum:    Option<String>,
    pub proprietates: Vec<SicAttr>,
    pub ns:           Option<usize>, /* index in propria ns_defs vel parentis */
    pub ns_defs:      Vec<SicNs>,
    pub filii:        Vec<SicNode>,
}

#[derive(Debug)]
pub struct SicDoc {
    pub versio:     String,
    pub codificatio: Option<String>,
    pub standalone: bool,
    pub filii:      Vec<SicNode>,
}

/* ================================================================
 * SicNs
 * ================================================================ */

impl SicNs {
    pub fn crea(href: Option<&str>, praefixum: Option<&str>) -> Self {
        SicNs {
            href:   href.map(String::from),
            prefix: praefixum.map(String::from),
        }
    }
}

/* ================================================================
 * SicNode
 * ================================================================ */

impl SicNode {
    /* crea nodum elementi */
    pub fn novus_nodus(nomen: &str) -> Self {
        SicNode {
            genus:        SicElementType::ElementNode,
            nomen:        Some(nomen.to_string()),
            contentum:    None,
            proprietates: Vec::new(),
            ns:           None,
            ns_defs:      Vec::new(),
            filii:        Vec::new(),
        }
    }

    /* crea nodum textus */
    pub fn novus_textus(contentum: &str) -> Self {
        SicNode {
            genus:        SicElementType::TextNode,
            nomen:        None,
            contentum:    Some(contentum.to_string()),
            proprietates: Vec::new(),
            ns:           None,
            ns_defs:      Vec::new(),
            filii:        Vec::new(),
        }
    }

    /* crea nodum CDATA */
    pub fn novus_cdata(contentum: &str) -> Self {
        SicNode {
            genus:        SicElementType::CdataSectionNode,
            nomen:        None,
            contentum:    Some(contentum.to_string()),
            proprietates: Vec::new(),
            ns:           None,
            ns_defs:      Vec::new(),
            filii:        Vec::new(),
        }
    }

    /* adde filium */
    pub fn adde_filium(&mut self, filius: SicNode) {
        self.filii.push(filius);
    }

    /* crea filium cum contentum et adde */
    pub fn novus_filius(
        &mut self,
        nomen: &str,
        contentum: Option<&str>,
    ) -> usize {
        let mut nodus = SicNode::novus_nodus(nomen);
        if let Some(c) = contentum {
            nodus.filii.push(SicNode::novus_textus(c));
        }
        self.filii.push(nodus);
        self.filii.len() - 1
    }

    /* insere ante fratrem ad indicem */
    pub fn insere_ante(&mut self, index: usize, nodus: SicNode) {
        if index <= self.filii.len() {
            self.filii.insert(index, nodus);
        }
    }

    /* tolle filium ad indicem */
    pub fn tolle_filium(&mut self, index: usize) -> Option<SicNode> {
        if index < self.filii.len() {
            Some(self.filii.remove(index))
        } else {
            None
        }
    }

    /* lege attributum */
    pub fn da_prop(&self, nomen: &str) -> Option<&str> {
        for a in &self.proprietates {
            if a.ns.is_none() && a.nomen == nomen {
                return a.valor.as_deref();
            }
        }
        None
    }

    /* lege attributum cum spatio nominum (per indicem ns) */
    pub fn da_ns_prop(&self, nomen: &str, ns_index: usize) -> Option<&str> {
        for a in &self.proprietates {
            if a.nomen == nomen && a.ns == Some(ns_index) {
                return a.valor.as_deref();
            }
        }
        None
    }

    /* pone attributum */
    pub fn pone_prop(&mut self, nomen: &str, valor: &str) {
        for a in &mut self.proprietates {
            if a.ns.is_none() && a.nomen == nomen {
                a.valor = Some(valor.to_string());
                return;
            }
        }
        self.proprietates.push(SicAttr {
            nomen: nomen.to_string(),
            valor: Some(valor.to_string()),
            ns:    None,
        });
    }

    /* pone attributum cum spatio nominum */
    pub fn pone_ns_prop(&mut self, ns_index: usize, nomen: &str, valor: &str) {
        for a in &mut self.proprietates {
            if a.nomen == nomen && a.ns == Some(ns_index) {
                a.valor = Some(valor.to_string());
                return;
            }
        }
        self.proprietates.push(SicAttr {
            nomen: nomen.to_string(),
            valor: Some(valor.to_string()),
            ns:    Some(ns_index),
        });
    }

    /* tolle attributum */
    pub fn tolle_prop(&mut self, nomen: &str) -> bool {
        let lon = self.proprietates.len();
        self.proprietates.retain(|a| !(a.ns.is_none() && a.nomen == nomen));
        self.proprietates.len() < lon
    }

    /* adde spatium nominum */
    pub fn adde_ns(&mut self, href: Option<&str>, praefixum: Option<&str>) -> usize {
        self.ns_defs.push(SicNs::crea(href, praefixum));
        self.ns_defs.len() - 1
    }

    /* pone spatium nominum nodi */
    pub fn pone_ns(&mut self, ns_index: usize) {
        self.ns = Some(ns_index);
    }

    /* quaere spatium nominum per URI in hoc nodo */
    pub fn quaere_ns_per_href(&self, href: &str) -> Option<usize> {
        for (i, ns) in self.ns_defs.iter().enumerate() {
            if ns.href.as_deref() == Some(href) {
                return Some(i);
            }
        }
        None
    }

    /* collige textum ex omnibus descendentibus */
    pub fn da_contentum(&self) -> String {
        match self.genus {
            SicElementType::TextNode | SicElementType::CdataSectionNode => {
                self.contentum.clone().unwrap_or_default()
            }
            _ => {
                let mut alveus = String::new();
                self.collige_textum(&mut alveus);
                alveus
            }
        }
    }

    fn collige_textum(&self, alveus: &mut String) {
        for filius in &self.filii {
            match filius.genus {
                SicElementType::TextNode | SicElementType::CdataSectionNode => {
                    if let Some(ref c) = filius.contentum {
                        alveus.push_str(c);
                    }
                }
                SicElementType::ElementNode => {
                    filius.collige_textum(alveus);
                }
                _ => {}
            }
        }
    }

    /* quaere primum filium per nomen */
    pub fn filius_per_nomen(&self, nomen: &str) -> Option<&SicNode> {
        self.filii.iter().find(|f| {
            f.genus == SicElementType::ElementNode
                && f.nomen.as_deref() == Some(nomen)
        })
    }

    /* quaere primum filium per nomen (mutabilis) */
    pub fn filius_per_nomen_mut(&mut self, nomen: &str) -> Option<&mut SicNode> {
        self.filii.iter_mut().find(|f| {
            f.genus == SicElementType::ElementNode
                && f.nomen.as_deref() == Some(nomen)
        })
    }

    /* da omnes filios elementorum */
    pub fn filii_elem(&self) -> impl Iterator<Item = &SicNode> {
        self.filii
            .iter()
            .filter(|f| f.genus == SicElementType::ElementNode)
    }
}

/* ================================================================
 * SicDoc
 * ================================================================ */

impl SicDoc {
    pub fn novus(versio: Option<&str>) -> Self {
        SicDoc {
            versio:      versio.unwrap_or("1.0").to_string(),
            codificatio: None,
            standalone:  false,
            filii:       Vec::new(),
        }
    }

    /* da radicem documenti */
    pub fn da_radicem(&self) -> Option<&SicNode> {
        self.filii
            .iter()
            .find(|n| n.genus == SicElementType::ElementNode)
    }

    /* da radicem documenti (mutabilis) */
    pub fn da_radicem_mut(&mut self) -> Option<&mut SicNode> {
        self.filii
            .iter_mut()
            .find(|n| n.genus == SicElementType::ElementNode)
    }

    /* pone radicem documenti — reddit veterem si adest */
    pub fn pone_radicem(&mut self, radix: SicNode) -> Option<SicNode> {
        let pos = self.filii.iter().position(|n| {
            n.genus == SicElementType::ElementNode
        });
        if let Some(i) = pos {
            let vetus = std::mem::replace(&mut self.filii[i], radix);
            Some(vetus)
        } else {
            self.filii.push(radix);
            None
        }
    }

    /* lege plicam XML */
    pub fn lege_plicam(via: &str, optiones: i32) -> Option<Self> {
        let data = fs::read(via).ok()?;
        Self::lege_chordas(&data, optiones)
    }

    /* disseca XML ex octis */
    pub fn lege_chordas(data: &[u8], optiones: i32) -> Option<Self> {
        /* praeterire BOM UTF-8 */
        let fons = if data.len() >= 3
            && data[0] == 0xEF
            && data[1] == 0xBB
            && data[2] == 0xBF
        {
            &data[3..]
        } else {
            data
        };

        let fons = std::str::from_utf8(fons).ok()?;
        let mut doc = SicDoc::novus(None);
        let mut ctx = DCtx::novus(fons, optiones);

        ctx.praeter_spatia();
        ctx.lege_xml_decl(&mut doc);

        /* praeterire miscella ante radicem */
        loop {
            ctx.praeter_spatia();
            if ctx.finis() {
                break;
            }
            if ctx.spectat("<!--") {
                ctx.praeter_commentarium();
            } else if ctx.spectat("<?") {
                ctx.praeter_pi();
            } else if ctx.spectat("<!DOCTYPE") {
                ctx.cur += 9;
                ctx.praeter_doctype();
            } else {
                break;
            }
        }

        /* disseca radicem */
        if !ctx.finis() && ctx.aspice() == Some('<') {
            ctx.cur += 1;
            if let Some(radix) = ctx.lege_elementum(optiones, &[]) {
                doc.filii.push(radix);
            }
        }

        Some(doc)
    }

    /* scribe documentum in chordam */
    pub fn in_chordam(&self, codificatio: Option<&str>, forma: bool) -> String {
        let mut alveus = Vec::new();
        self.scribe(&mut alveus, codificatio, forma).unwrap();
        String::from_utf8(alveus).unwrap()
    }

    /* scribe documentum in plicam */
    pub fn serva_in_plicam(
        &self,
        via: &str,
        codificatio: Option<&str>,
        forma: bool,
    ) -> io::Result<()> {
        let mut fp = fs::File::create(via)?;
        self.scribe(&mut fp, codificatio, forma)
    }

    /* scribe documentum in scriptor */
    pub fn scribe<W: Write>(
        &self,
        w: &mut W,
        codificatio: Option<&str>,
        forma: bool,
    ) -> io::Result<()> {
        write!(w, "<?xml version=\"{}\"", self.versio)?;
        if let Some(cod) = codificatio {
            write!(w, " encoding=\"{}\"", cod)?;
        } else if let Some(ref cod) = self.codificatio {
            write!(w, " encoding=\"{}\"", cod)?;
        }
        write!(w, "?>\n")?;

        for filius in &self.filii {
            scribe_nodum(w, filius, 0, forma, &[])?;
        }
        if forma {
            write!(w, "\n")?;
        }
        Ok(())
    }
}

/* ================================================================
 * scriptor — arbor in XML
 * ================================================================ */

fn scribe_escaped<W: Write>(w: &mut W, s: &str, attr: bool) -> io::Result<()> {
    for c in s.bytes() {
        match c {
            b'&' => write!(w, "&amp;")?,
            b'<' => write!(w, "&lt;")?,
            b'>' if !attr => write!(w, "&gt;")?,
            b'"' if attr => write!(w, "&quot;")?,
            _ => w.write_all(&[c])?,
        }
    }
    Ok(())
}

/* collige omnes declarationes ns ab hoc nodo et antecedentibus */
fn collige_ns_ctx<'a>(
    nodus: &'a SicNode,
    parentes_ns: &[&'a SicNs],
) -> Vec<&'a SicNs> {
    let mut res: Vec<&SicNs> = parentes_ns.to_vec();
    for ns in &nodus.ns_defs {
        res.push(ns);
    }
    res
}

fn resolve_ns_prefix(ns_index: usize, nodus: &SicNode, omnes_ns: &[&SicNs]) -> Option<String> {
    /* primum quaere in nodo ipso */
    if ns_index < nodus.ns_defs.len() {
        return nodus.ns_defs[ns_index].prefix.clone();
    }
    /* deinde in contextu parentum */
    if ns_index < omnes_ns.len() {
        return omnes_ns[ns_index].prefix.clone();
    }
    None
}

fn scribe_nodum<W: Write>(
    w: &mut W,
    nodus: &SicNode,
    altitudo: usize,
    forma: bool,
    parentes_ns: &[&SicNs],
) -> io::Result<()> {
    match nodus.genus {
        SicElementType::TextNode | SicElementType::CdataSectionNode => {
            if let Some(ref c) = nodus.contentum {
                scribe_escaped(w, c, false)?;
            }
            return Ok(());
        }
        SicElementType::ElementNode => {}
        _ => return Ok(()),
    }

    let omnes_ns = collige_ns_ctx(nodus, parentes_ns);

    if forma && altitudo > 0 {
        write!(w, "\n")?;
        for _ in 0..altitudo {
            write!(w, "  ")?;
        }
    }

    write!(w, "<")?;

    /* praefixum spatii nominum */
    if let Some(ns_i) = nodus.ns {
        if let Some(ref praef) = resolve_ns_prefix(ns_i, nodus, &omnes_ns) {
            write!(w, "{}:", praef)?;
        }
    }
    write!(w, "{}", nodus.nomen.as_deref().unwrap_or(""))?;

    /* declarationes spatiorum nominum */
    for ns in &nodus.ns_defs {
        if let Some(ref praef) = ns.prefix {
            write!(w, " xmlns:{}=\"", praef)?;
        } else {
            write!(w, " xmlns=\"")?;
        }
        if let Some(ref href) = ns.href {
            scribe_escaped(w, href, true)?;
        }
        write!(w, "\"")?;
    }

    /* attributa */
    for a in &nodus.proprietates {
        write!(w, " ")?;
        if let Some(ns_i) = a.ns {
            if let Some(ref praef) = resolve_ns_prefix(ns_i, nodus, &omnes_ns) {
                write!(w, "{}:", praef)?;
            }
        }
        write!(w, "{}=\"", a.nomen)?;
        if let Some(ref v) = a.valor {
            scribe_escaped(w, v, true)?;
        }
        write!(w, "\"")?;
    }

    if nodus.filii.is_empty() {
        write!(w, "/>")?;
        return Ok(());
    }

    write!(w, ">")?;

    /* habet filios elementorum? */
    let habet_elem = nodus
        .filii
        .iter()
        .any(|f| f.genus == SicElementType::ElementNode);

    for filius in &nodus.filii {
        scribe_nodum(w, filius, altitudo + 1, forma && habet_elem, &omnes_ns)?;
    }

    if forma && habet_elem {
        write!(w, "\n")?;
        for _ in 0..altitudo {
            write!(w, "  ")?;
        }
    }

    write!(w, "</")?;
    if let Some(ns_i) = nodus.ns {
        if let Some(ref praef) = resolve_ns_prefix(ns_i, nodus, &omnes_ns) {
            write!(w, "{}:", praef)?;
        }
    }
    write!(w, "{}>", nodus.nomen.as_deref().unwrap_or(""))?;

    Ok(())
}

/* ================================================================
 * dissector — XML in arborem
 * ================================================================ */

struct DCtx<'a> {
    fons: &'a [u8],
    cur:  usize,
}

impl<'a> DCtx<'a> {
    fn novus(fons: &'a str, _optiones: i32) -> Self {
        DCtx {
            fons: fons.as_bytes(),
            cur:  0,
        }
    }

    fn finis(&self) -> bool {
        self.cur >= self.fons.len()
    }

    fn aspice(&self) -> Option<char> {
        if self.finis() {
            None
        } else {
            Some(self.fons[self.cur] as char)
        }
    }

    fn ede(&mut self) -> Option<u8> {
        if self.finis() {
            None
        } else {
            let c = self.fons[self.cur];
            self.cur += 1;
            Some(c)
        }
    }

    fn praeter_spatia(&mut self) {
        while !self.finis() && (self.fons[self.cur] as char).is_ascii_whitespace() {
            self.cur += 1;
        }
    }

    fn spectat(&self, s: &str) -> bool {
        let b = s.as_bytes();
        self.cur + b.len() <= self.fons.len()
            && &self.fons[self.cur..self.cur + b.len()] == b
    }

    fn congruit(&mut self, s: &str) -> bool {
        if self.spectat(s) {
            self.cur += s.len();
            true
        } else {
            false
        }
    }

    /* --- nomina XML --- */

    fn est_nomen_c(c: u8, primus: bool) -> bool {
        if c.is_ascii_alphabetic() || c == b'_' {
            return true;
        }
        if primus {
            return false;
        }
        c.is_ascii_digit() || c == b'-' || c == b'.' || c == b':'
    }

    fn lege_nomen(&mut self) -> Option<String> {
        if self.finis() || !Self::est_nomen_c(self.fons[self.cur], true) {
            return None;
        }
        let initium = self.cur;
        while !self.finis() && Self::est_nomen_c(self.fons[self.cur], false) {
            self.cur += 1;
        }
        Some(
            String::from_utf8_lossy(&self.fons[initium..self.cur]).into_owned(),
        )
    }

    /* --- entitates --- */

    fn lege_entitas(&mut self, alveus: &mut Vec<u8>) {
        self.cur += 1; /* praeterire '&' */

        if self.spectat("amp;") {
            self.cur += 4;
            alveus.push(b'&');
            return;
        }
        if self.spectat("lt;") {
            self.cur += 3;
            alveus.push(b'<');
            return;
        }
        if self.spectat("gt;") {
            self.cur += 3;
            alveus.push(b'>');
            return;
        }
        if self.spectat("quot;") {
            self.cur += 5;
            alveus.push(b'"');
            return;
        }
        if self.spectat("apos;") {
            self.cur += 5;
            alveus.push(b'\'');
            return;
        }

        if self.aspice() == Some('#') {
            self.cur += 1;
            let mut codex: u32 = 0;
            if self.aspice() == Some('x') {
                self.cur += 1;
                while !self.finis() && self.aspice() != Some(';') {
                    let c = self.ede().unwrap();
                    match c {
                        b'0'..=b'9' => codex = codex * 16 + (c - b'0') as u32,
                        b'a'..=b'f' => codex = codex * 16 + (c - b'a' + 10) as u32,
                        b'A'..=b'F' => codex = codex * 16 + (c - b'A' + 10) as u32,
                        _ => {}
                    }
                }
            } else {
                while !self.finis() && self.aspice() != Some(';') {
                    let c = self.fons[self.cur];
                    if c >= b'0' && c <= b'9' {
                        codex = codex * 10 + (c - b'0') as u32;
                    }
                    self.cur += 1;
                }
            }
            if !self.finis() {
                self.cur += 1; /* ';' */
            }

            /* codifica UTF-8 */
            if codex < 0x80 {
                alveus.push(codex as u8);
            } else if codex < 0x800 {
                alveus.push((0xC0 | (codex >> 6)) as u8);
                alveus.push((0x80 | (codex & 0x3F)) as u8);
            } else if codex < 0x10000 {
                alveus.push((0xE0 | (codex >> 12)) as u8);
                alveus.push((0x80 | ((codex >> 6) & 0x3F)) as u8);
                alveus.push((0x80 | (codex & 0x3F)) as u8);
            } else {
                alveus.push((0xF0 | (codex >> 18)) as u8);
                alveus.push((0x80 | ((codex >> 12) & 0x3F)) as u8);
                alveus.push((0x80 | ((codex >> 6) & 0x3F)) as u8);
                alveus.push((0x80 | (codex & 0x3F)) as u8);
            }
            return;
        }

        /* entitas ignota — emitte '&' ut est */
        alveus.push(b'&');
    }

    /* --- valor attributi inter apices --- */

    fn lege_valor_attr(&mut self) -> Option<String> {
        let apex = self.ede()?;
        if apex != b'"' && apex != b'\'' {
            return None;
        }
        let mut alveus = Vec::new();
        while !self.finis() && self.fons[self.cur] != apex {
            if self.fons[self.cur] == b'&' {
                self.lege_entitas(&mut alveus);
            } else {
                alveus.push(self.ede().unwrap());
            }
        }
        if !self.finis() {
            self.cur += 1; /* apex claudens */
        }
        Some(String::from_utf8_lossy(&alveus).into_owned())
    }

    /* --- praeterire commentaria, PI, DOCTYPE --- */

    fn praeter_commentarium(&mut self) {
        self.cur += 4; /* "<!--" */
        while !self.finis() && !self.spectat("-->") {
            self.cur += 1;
        }
        if self.spectat("-->") {
            self.cur += 3;
        }
    }

    fn praeter_pi(&mut self) {
        self.cur += 2; /* "<?" */
        while !self.finis() && !self.spectat("?>") {
            self.cur += 1;
        }
        if self.spectat("?>") {
            self.cur += 2;
        }
    }

    fn praeter_doctype(&mut self) {
        let mut altitudo = 0i32;
        while !self.finis() {
            let c = self.fons[self.cur];
            match c {
                b'[' => {
                    altitudo += 1;
                    self.cur += 1;
                }
                b']' => {
                    altitudo -= 1;
                    self.cur += 1;
                }
                b'>' if altitudo == 0 => {
                    self.cur += 1;
                    return;
                }
                _ => {
                    self.cur += 1;
                }
            }
        }
    }

    /* --- CDATA --- */

    fn lege_cdata(&mut self) -> SicNode {
        self.cur += 9; /* "<![CDATA[" */
        let initium = self.cur;
        while !self.finis() && !self.spectat("]]>") {
            self.cur += 1;
        }
        let contentum =
            String::from_utf8_lossy(&self.fons[initium..self.cur]).into_owned();
        if self.spectat("]]>") {
            self.cur += 3;
        }
        SicNode::novus_cdata(&contentum)
    }

    /* --- nodus textus --- */

    fn lege_textus(&mut self) -> Option<SicNode> {
        let mut alveus = Vec::new();
        while !self.finis() && self.fons[self.cur] != b'<' {
            if self.fons[self.cur] == b'&' {
                self.lege_entitas(&mut alveus);
            } else {
                alveus.push(self.ede().unwrap());
            }
        }
        if alveus.is_empty() {
            return None;
        }
        let s = String::from_utf8_lossy(&alveus).into_owned();
        Some(SicNode {
            genus:        SicElementType::TextNode,
            nomen:        None,
            contentum:    Some(s),
            proprietates: Vec::new(),
            ns:           None,
            ns_defs:      Vec::new(),
            filii:        Vec::new(),
        })
    }

    /* --- declaratio XML --- */

    fn lege_xml_decl(&mut self, doc: &mut SicDoc) {
        if !self.spectat("<?xml") {
            return;
        }
        self.cur += 5;
        while !self.finis() && !self.spectat("?>") {
            self.praeter_spatia();
            if self.spectat("?>") {
                break;
            }
            let nom = self.lege_nomen();
            self.praeter_spatia();
            if self.aspice() == Some('=') {
                self.cur += 1;
            }
            self.praeter_spatia();
            let val = self.lege_valor_attr();
            if let (Some(ref n), Some(ref v)) = (&nom, &val) {
                match n.as_str() {
                    "version" => doc.versio = v.clone(),
                    "encoding" => doc.codificatio = Some(v.clone()),
                    "standalone" => doc.standalone = v == "yes",
                    _ => {}
                }
            }
        }
        if self.spectat("?>") {
            self.cur += 2;
        }
    }

    /* --- disseca elementum (post '<' iam consumptum) --- */

    fn lege_elementum(
        &mut self,
        optiones: i32,
        parentes_ns: &[SicNs],
    ) -> Option<SicNode> {
        let nomen_plenum = self.lege_nomen()?;

        /* lege attributa in indicem temporalem */
        let mut paria: Vec<(String, Option<String>)> = Vec::new();
        loop {
            self.praeter_spatia();
            match self.aspice() {
                Some('/') | Some('>') | None => break,
                _ => {}
            }
            let an = match self.lege_nomen() {
                Some(n) => n,
                None => break,
            };
            self.praeter_spatia();
            if self.aspice() == Some('=') {
                self.cur += 1;
            }
            self.praeter_spatia();
            let av = self.lege_valor_attr();
            paria.push((an, av));
        }

        let mut nodus = SicNode {
            genus:        SicElementType::ElementNode,
            nomen:        None,
            contentum:    None,
            proprietates: Vec::new(),
            ns:           None,
            ns_defs:      Vec::new(),
            filii:        Vec::new(),
        };

        /* primo transitu: processa declarationes spatiorum nominum */
        let mut processati = vec![false; paria.len()];
        for (i, (nom, val)) in paria.iter().enumerate() {
            if nom == "xmlns" {
                nodus.ns_defs.push(SicNs {
                    href:   val.clone(),
                    prefix: None,
                });
                processati[i] = true;
            } else if let Some(praef) = nom.strip_prefix("xmlns:") {
                nodus.ns_defs.push(SicNs {
                    href:   val.clone(),
                    prefix: Some(praef.to_string()),
                });
                processati[i] = true;
            }
        }

        /* resolve nomen elementi */
        if let Some(pos) = nomen_plenum.find(':') {
            let praef = &nomen_plenum[..pos];
            let locale = &nomen_plenum[pos + 1..];
            nodus.nomen = Some(locale.to_string());
            nodus.ns = Self::quaere_ns_praefixum_ctx(
                &nodus.ns_defs,
                parentes_ns,
                Some(praef),
            );
        } else {
            nodus.nomen = Some(nomen_plenum);
            nodus.ns = Self::quaere_ns_praefixum_ctx(
                &nodus.ns_defs,
                parentes_ns,
                None,
            );
        }

        /* secundo transitu: crea attributa */
        for (i, (nom, val)) in paria.into_iter().enumerate() {
            if processati[i] {
                continue;
            }
            if let Some(pos) = nom.find(':') {
                let praef = &nom[..pos];
                let locale = &nom[pos + 1..];
                let ns_i = Self::quaere_ns_praefixum_ctx(
                    &nodus.ns_defs,
                    parentes_ns,
                    Some(praef),
                );
                nodus.proprietates.push(SicAttr {
                    nomen: locale.to_string(),
                    valor: val,
                    ns:    ns_i,
                });
            } else {
                nodus.proprietates.push(SicAttr {
                    nomen: nom,
                    valor: val,
                    ns:    None,
                });
            }
        }

        /* elementum clausum vel apertum? */
        if self.congruit("/>") {
            return Some(nodus);
        }
        if !self.finis() {
            self.cur += 1; /* '>' */
        }

        /* disseca filios — combina ns parentum cum nodi */
        let mut ns_filii: Vec<SicNs> = parentes_ns.to_vec();
        ns_filii.extend(nodus.ns_defs.iter().cloned());
        self.lege_contentum(&mut nodus, optiones, &ns_filii);

        /* lege signum claudens </nomen> */
        if self.congruit("</") {
            let _ = self.lege_nomen();
            self.praeter_spatia();
            if self.aspice() == Some('>') {
                self.cur += 1;
            }
        }

        Some(nodus)
    }

    /* disseca contentum inter signa aperientia et claudentia */
    fn lege_contentum(
        &mut self,
        parens: &mut SicNode,
        optiones: i32,
        parentes_ns: &[SicNs],
    ) {
        while !self.finis() {
            if self.fons[self.cur] == b'<' {
                if self.spectat("</") {
                    return;
                }
                if self.spectat("<!--") {
                    self.praeter_commentarium();
                    continue;
                }
                if self.spectat("<![CDATA[") {
                    let c = self.lege_cdata();
                    parens.filii.push(c);
                    continue;
                }
                if self.spectat("<?") {
                    self.praeter_pi();
                    continue;
                }
                /* elementum filium */
                self.cur += 1; /* '<' */
                if let Some(c) = self.lege_elementum(optiones, parentes_ns) {
                    parens.filii.push(c);
                }
            } else {
                if let Some(t) = self.lege_textus() {
                    if (optiones & SIC_PARSE_NOBLANKS) != 0 && est_vacua(&t) {
                        /* praetermitte */
                    } else {
                        parens.filii.push(t);
                    }
                }
            }
        }
    }

    /* quaere spatium nominum per praefixum in nodo et contextu parentum */
    fn quaere_ns_praefixum_ctx(
        nodi_ns: &[SicNs],
        parentes_ns: &[SicNs],
        praef: Option<&str>,
    ) -> Option<usize> {
        /* primum quaere in nodo ipso */
        for (i, ns) in nodi_ns.iter().enumerate() {
            match (praef, &ns.prefix) {
                (None, None) => return Some(i),
                (Some(p), Some(ref np)) if p == np => return Some(i),
                _ => {}
            }
        }
        /* deinde in contextu parentum */
        for (i, ns) in parentes_ns.iter().enumerate() {
            match (praef, &ns.prefix) {
                (None, None) => return Some(i),
                (Some(p), Some(ref np)) if p == np => return Some(i),
                _ => {}
            }
        }
        None
    }
}

/* est chorda tota ex spatiis? */
fn est_vacua(nodus: &SicNode) -> bool {
    match &nodus.contentum {
        None => true,
        Some(s) => s.bytes().all(|c| (c as char).is_ascii_whitespace()),
    }
}

/* ================================================================
 * Display
 * ================================================================ */

impl fmt::Display for SicDoc {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.in_chordam(None, true))
    }
}

/* ================================================================
 * probationes
 * ================================================================ */

#[cfg(test)]
mod probationes {
    use super::*;

    #[test]
    fn proba_crea_documentum() {
        let mut doc = SicDoc::novus(Some("1.0"));
        let mut radix = SicNode::novus_nodus("workbook");

        let ns_sml = radix.adde_ns(
            Some("http://schemas.example.com/spreadsheetml/main"),
            None,
        );
        let ns_r = radix.adde_ns(
            Some("http://schemas.example.com/relationships"),
            Some("r"),
        );
        radix.pone_ns(ns_sml);

        let idx = radix.novus_filius("sheets", None);
        {
            let sheets = &mut radix.filii[idx];
            let si = sheets.novus_filius("sheet", None);
            {
                let sh1 = &mut sheets.filii[si];
                sh1.pone_prop("name", "Sheet1");
                sh1.pone_prop("sheetId", "1");
                sh1.pone_ns_prop(ns_r, "id", "rId1");
            }
            let si2 = sheets.novus_filius("sheet", None);
            {
                let sh2 = &mut sheets.filii[si2];
                sh2.pone_prop("name", "Sheet2");
                sh2.pone_prop("sheetId", "2");
                sh2.pone_ns_prop(ns_r, "id", "rId2");
            }
        }

        doc.pone_radicem(radix);

        let xml = doc.in_chordam(Some("UTF-8"), true);
        assert!(xml.contains("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"));
        assert!(xml.contains("workbook"));
        assert!(xml.contains("xmlns="));
        assert!(xml.contains("Sheet1"));
        assert!(xml.contains("Sheet2"));
        assert!(xml.contains("r:id=\"rId1\""));
    }

    #[test]
    fn proba_disseca_simplex() {
        let xml = r#"<?xml version="1.0"?>
<radix><filius attr="val">textus</filius></radix>"#;
        let doc =
            SicDoc::lege_chordas(xml.as_bytes(), 0).expect("dissectio falli non debet");
        let radix = doc.da_radicem().expect("radix deest");
        assert_eq!(radix.nomen.as_deref(), Some("radix"));
        assert_eq!(radix.filii.len(), 1);

        let filius = &radix.filii[0];
        assert_eq!(filius.nomen.as_deref(), Some("filius"));
        assert_eq!(filius.da_prop("attr"), Some("val"));
        assert_eq!(filius.da_contentum(), "textus");
    }

    #[test]
    fn proba_disseca_spatia_nominum() {
        let xml = r#"<root xmlns="http://example.com" xmlns:ex="http://ex.com">
  <ex:child ex:id="1">text</ex:child>
</root>"#;
        let doc = SicDoc::lege_chordas(xml.as_bytes(), SIC_PARSE_NOBLANKS)
            .expect("dissectio falli non debet");
        let radix = doc.da_radicem().unwrap();
        assert_eq!(radix.nomen.as_deref(), Some("root"));
        assert_eq!(radix.ns_defs.len(), 2);

        let filius = &radix.filii[0];
        assert_eq!(filius.nomen.as_deref(), Some("child"));
        assert!(filius.ns.is_some());
        assert_eq!(filius.da_contentum(), "text");
    }

    #[test]
    fn proba_entitates() {
        let xml = r#"<r>&amp;&lt;&gt;&quot;&apos;&#65;&#x42;</r>"#;
        let doc = SicDoc::lege_chordas(xml.as_bytes(), 0).unwrap();
        let radix = doc.da_radicem().unwrap();
        let contentum = radix.da_contentum();
        assert_eq!(contentum, "&<>\"'AB");
    }

    #[test]
    fn proba_cdata() {
        let xml = r#"<r><![CDATA[<non est XML>]]></r>"#;
        let doc = SicDoc::lege_chordas(xml.as_bytes(), 0).unwrap();
        let radix = doc.da_radicem().unwrap();
        assert_eq!(radix.da_contentum(), "<non est XML>");
    }

    #[test]
    fn proba_noblanks() {
        let xml = "<?xml version=\"1.0\"?>\n<r>\n  <a/>\n  <b/>\n</r>";
        let doc =
            SicDoc::lege_chordas(xml.as_bytes(), SIC_PARSE_NOBLANKS).unwrap();
        let radix = doc.da_radicem().unwrap();
        /* sine NOBLANKS habet 5 filios (textus + elem); cum NOBLANKS solum 2 */
        assert_eq!(radix.filii.len(), 2);
    }

    #[test]
    fn proba_elementum_clausum() {
        let xml = r#"<r><vacuum/></r>"#;
        let doc = SicDoc::lege_chordas(xml.as_bytes(), 0).unwrap();
        let radix = doc.da_radicem().unwrap();
        let filius = &radix.filii[0];
        assert_eq!(filius.nomen.as_deref(), Some("vacuum"));
        assert!(filius.filii.is_empty());
    }

    #[test]
    fn proba_manipulatio() {
        let mut nodus = SicNode::novus_nodus("test");
        nodus.pone_prop("a", "1");
        nodus.pone_prop("b", "2");
        assert_eq!(nodus.da_prop("a"), Some("1"));
        assert_eq!(nodus.da_prop("b"), Some("2"));

        /* muta */
        nodus.pone_prop("a", "nova");
        assert_eq!(nodus.da_prop("a"), Some("nova"));

        /* tolle */
        assert!(nodus.tolle_prop("b"));
        assert_eq!(nodus.da_prop("b"), None);
    }

    #[test]
    fn proba_iter_rotundum() {
        let xml_fons = r#"<?xml version="1.0" encoding="UTF-8"?>
<catalog>
  <book id="1">
    <title>Liber Primus</title>
    <author>Auctor</author>
  </book>
  <book id="2">
    <title>Liber Secundus</title>
  </book>
</catalog>"#;
        let doc = SicDoc::lege_chordas(xml_fons.as_bytes(), SIC_PARSE_NOBLANKS)
            .unwrap();
        let xml_exit = doc.in_chordam(Some("UTF-8"), true);
        assert!(xml_exit.contains("<catalog>"));
        assert!(xml_exit.contains("Liber Primus"));
        assert!(xml_exit.contains("Liber Secundus"));
        assert!(xml_exit.contains("id=\"1\""));
        assert!(xml_exit.contains("id=\"2\""));
    }

    #[test]
    fn proba_bom_utf8() {
        let mut data = vec![0xEF, 0xBB, 0xBF];
        data.extend_from_slice(b"<r>salve</r>");
        let doc = SicDoc::lege_chordas(&data, 0).unwrap();
        let radix = doc.da_radicem().unwrap();
        assert_eq!(radix.da_contentum(), "salve");
    }

    #[test]
    fn proba_commentarium_et_pi() {
        let xml = r#"<?xml version="1.0"?>
<!-- commentarium -->
<?pi target?>
<r>val</r>"#;
        let doc = SicDoc::lege_chordas(xml.as_bytes(), 0).unwrap();
        let radix = doc.da_radicem().unwrap();
        assert_eq!(radix.nomen.as_deref(), Some("r"));
        assert_eq!(radix.da_contentum(), "val");
    }

    #[test]
    fn proba_pone_radicem() {
        let mut doc = SicDoc::novus(Some("1.0"));
        let r1 = SicNode::novus_nodus("prima");
        assert!(doc.pone_radicem(r1).is_none());
        assert_eq!(
            doc.da_radicem().unwrap().nomen.as_deref(),
            Some("prima")
        );

        let r2 = SicNode::novus_nodus("secunda");
        let vetus = doc.pone_radicem(r2);
        assert!(vetus.is_some());
        assert_eq!(vetus.unwrap().nomen.as_deref(), Some("prima"));
        assert_eq!(
            doc.da_radicem().unwrap().nomen.as_deref(),
            Some("secunda")
        );
    }

    #[test]
    fn proba_filius_per_nomen() {
        let mut radix = SicNode::novus_nodus("r");
        radix.novus_filius("alpha", None);
        radix.novus_filius("beta", Some("contentum"));
        radix.novus_filius("gamma", None);

        assert!(radix.filius_per_nomen("alpha").is_some());
        assert!(radix.filius_per_nomen("beta").is_some());
        assert_eq!(
            radix.filius_per_nomen("beta").unwrap().da_contentum(),
            "contentum"
        );
        assert!(radix.filius_per_nomen("delta").is_none());
    }
}
