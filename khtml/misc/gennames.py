#!/usr/bin/python

cache = {}
namesList = []

# parse htmltags.in file
f = open("htmltags.in", "r")
for i in f.xreadlines():
    i = i.strip()
    if i.startswith("#") or i == "": continue
    if i == "": continue
    key = cache.get(i, len(cache) + 1)
    cache[i] = key
    namesList.append([key, i, "ID_%s" % i.upper().replace("-", "_")])
f.close()

lastTagId = len(cache)

# parse htmlattrs.in file
f = open("htmlattrs.in", "r")
lastCaseInsensitiveAttr = 0
additionalCaseSensitiveAttrs = []
for i in f.xreadlines():
    i = i.strip()
    if i.startswith("#") or i == "": continue
    if i.startswith("END_CI"):
        lastCaseInsensitiveAttr = len(cache)
        continue
    if i.upper() == "TEXT":
        key = len(cache) + 1
        cache["TEXT#"] = -1 # hack to get ID_TEXT and ATTR_TEXT be different
    else:
        key = cache.get(i, len(cache) + 1)
    if lastCaseInsensitiveAttr > 0 and key <= len(cache):
        additionalCaseSensitiveAttrs.append("ATTR_%s" % i.upper().replace("-", "_"))
    cache[i] = key
    namesList.append([key, i, "ATTR_%s" % i.upper().replace("-", "_")])
f.close()
lastAttrId = len(cache)

def func(a, b):
    return cmp(a[0], b[0]) or -cmp(a[2], b[2])
namesList.sort(func)

out = open("htmlnames.h", "w")
out.write("/* This file is automatically generated from htmltags.in and htmlattrs.in by gennames.py, do not edit */\n")
out.write("/* Copyright 2008 Vyacheslav Tokarev */\n")
out.write("\n")
out.write("#ifndef HTMLNames_h\n")
out.write("#define HTMLNames_h\n")
out.write("\n")
out.write("#include \"misc/idstring.h\"\n")
out.write("\n")
out.write("#define XHTML_NAMESPACE \"http://www.w3.org/1999/xhtml\"\n")
out.write("#define SVG_NAMESPACE \"http://www.w3.org/2000/svg\"\n")
out.write("\n")
for i in namesList:
    if i[2].startswith("ATTR"):
        out.write("#define %s %d\n" % (i[2], 65536 + i[0]))
    else:
        out.write("#define %s %d\n" % (i[2], i[0]))
out.write("#define ID_LAST_TAG %d\n" % (lastTagId))
out.write("#define ID_CLOSE_TAG 16384\n")
out.write("#define ATTR_LAST_ATTR %d\n" % (lastAttrId))
out.write("#define ATTR_LAST_CI_ATTR %d\n" % (lastCaseInsensitiveAttr))
out.write("\n")
s = "((localNamePart(id)) > ATTR_LAST_CI_ATTR"
for i in additionalCaseSensitiveAttrs:
    s = s + " || (id) == %s" % i
s = s + ")"
out.write("#define caseSensitiveAttr(id) (%s)\n" % s)
out.write("\n")
out.write("namespace DOM {\n\
\n\
#define NodeImpl_IdNSMask    0xffff0000\n\
#define NodeImpl_IdLocalMask 0x0000ffff\n\
\n\
const quint16 xhtmlNamespace = 0;\n\
const quint16 emptyNamespace = 1;\n\
const quint16 svgNamespace = 2;\n\
const quint16 anyNamespace = 0xffff;\n\
const quint16 anyLocalName = 0xffff;\n\
const quint16 emptyPrefix = 0;\n\
\n\
inline quint16 localNamePart(quint32 id) { return id & NodeImpl_IdLocalMask; }\n\
inline quint16 namespacePart(quint32 id) { return (((unsigned int)id) & NodeImpl_IdNSMask) >> 16; }\n\
inline quint32 makeId(quint16 n, quint16 l) { return (n << 16) | l; }\n\
\n\
const quint32 anyQName = makeId(anyNamespace, anyLocalName);\n\
\n\
}\n")
out.write("\n")
out.write("namespace khtml {\n\
\n\
class NamespaceFactory {\n\
public:\n\
    static IDTable<NamespaceFactory>* idTable() {\n\
    if (s_idTable)\n\
            return s_idTable;\n\
        else\n\
            return initIdTable();\n\
    }\n\
protected:\n\
    static IDTable<NamespaceFactory>* s_idTable;\n\
    static IDTable<NamespaceFactory>* initIdTable();\n\
};\n\
\n\
class LocalNameFactory {\n\
public:\n\
    static IDTable<LocalNameFactory>* idTable() {\n\
        if (s_idTable)\n\
            return s_idTable;\n\
        else\n\
            return initIdTable();\n\
    }\n\
protected:\n\
    static IDTable<LocalNameFactory>* s_idTable;\n\
    static IDTable<LocalNameFactory>* initIdTable();\n\
};\n\
\n\
class PrefixFactory {\n\
public:\n\
    static IDTable<PrefixFactory>* idTable() {\n\
        if (s_idTable)\n\
            return s_idTable;\n\
        else\n\
            return initIdTable();\n\
    }\n\
protected:\n\
    static IDTable<PrefixFactory>* s_idTable;\n\
    static IDTable<PrefixFactory>* initIdTable();\n\
};\n")
out.write("\n")
out.write("}\n")
out.write("\n")
out.write("namespace DOM {\n\
\n\
    typedef khtml::IDString<khtml::NamespaceFactory> NamespaceName;\n\
    typedef khtml::IDString<khtml::LocalNameFactory> LocalName;\n\
    typedef khtml::IDString<khtml::PrefixFactory> PrefixName;\n\
    extern PrefixName emptyPrefixName;\n\
    extern LocalName emptyLocalName;\n\
    extern NamespaceName emptyNamespaceName;\n\
\n\
    QString getPrintableName(int id);\n\n")
out.write("}\n")
out.write("#endif\n")
out.close()

temp = ""
prev = 0
for i in namesList:
    if prev and prev[0] == i[0]: continue
    if i[2] == "ID_TEXT" or i[2] == "ID_COMMENT":
        temp = temp + ("    s_idTable->addHiddenMapping(%s, \"%s\");\n" % (i[2], i[1]))
    else:
        temp = temp + ("    s_idTable->addStaticMapping(localNamePart(%s), \"%s\");\n" % (i[2], i[1]))
    prev = i

out = open("htmlnames.cpp", "w")
out.write("#include \"misc/htmlnames.h\"\n")
out.write("#include \"dom/dom_string.h\"\n")
out.write("\nusing namespace DOM;\n")
out.write("\n")
out.write("namespace khtml {\n\n")
out.write("IDTable<NamespaceFactory>* NamespaceFactory::s_idTable;\n\
IDTable<NamespaceFactory>* NamespaceFactory::initIdTable()\n\
{\n\
    s_idTable = new IDTable<NamespaceFactory>();\n\
    s_idTable->addStaticMapping(DOM::xhtmlNamespace, XHTML_NAMESPACE);\n\
    s_idTable->addStaticMapping(DOM::emptyNamespace, DOMString());\n\
    s_idTable->addStaticMapping(DOM::svgNamespace, SVG_NAMESPACE);\n\
    return s_idTable;\n\
}\n\
\n\
IDTable<LocalNameFactory>* LocalNameFactory::s_idTable;\n\
IDTable<LocalNameFactory>* LocalNameFactory::initIdTable()\n\
{\n\
    s_idTable = new IDTable<LocalNameFactory>();\n\
    s_idTable->addStaticMapping(0, DOMString());\n\
%s\
    return s_idTable;\n\
}\n\
\n\
IDTable<PrefixFactory>* PrefixFactory::s_idTable;\n\
IDTable<PrefixFactory>* PrefixFactory::initIdTable()\n\
{\n\
    s_idTable = new IDTable<PrefixFactory>();\n\
    s_idTable->addStaticMapping(DOM::emptyPrefix, DOMString());\n\
    return s_idTable;\n\
}\n" % temp)
out.write("\n}\n")
out.write("\n")
out.write("namespace DOM {\n\n")
out.write("LocalName emptyLocalName = LocalName::fromId(0);\n")
out.write("PrefixName emptyPrefixName = PrefixName::fromId(0);\n")
out.write("NamespaceName emptyNamespaceName = NamespaceName::fromId(0);\n")
out.write("\n")
out.write("""QString getPrintableName(int id) {
    QString local = QString("null");
    QString namespacename = QString("null");

    if (localNamePart(id) != anyLocalName) {
        DOMString localName = LocalName::fromId(localNamePart(id)).toString();
        if (localName.implementation())
            local = localName.string();
    } else {
        local = "*";
    }

    if (namespacePart(id) != anyNamespace) {
        DOMString namespaceName = NamespaceName::fromId(namespacePart(id)).toString();
        if (namespaceName.implementation())
            namespacename = namespaceName.string();
    } else {
        namespacename = "*";
    }
    return "{ns:" + QString::number(namespacePart(id)) + ",[" + namespacename + "] local:" + QString::number(localNamePart(id)) + ",[" + local + "]}";
}\n""")
out.write("\n}\n")
out.close()

