
def validate2(xsd_path,xml_path):
    import xmlschema
    my_schema = xmlschema.XMLSchema(xsd_path)
    my_schema.validate(xml_path)
    return my_schema.is_valid(xml_path)

def validate(xsd_path,xml_path):
    from lxml import etree
    xmlschema_doc = etree.parse(xsd_path)
    xmlschema = etree.XMLSchema(xmlschema_doc)

    xml_doc = etree.parse(xml_path)
    result = xmlschema.validate(xml_doc)

    return result

if __name__ == '__main__':
    import sys
    if len(sys.argv) != 3:
        print "validatexml: xsdfile xsmlfile"
    else:
        r = validate2(sys.argv[1],sys.argv[2])
        print "valid" if r else "invalid"
        sys.exit(0 if r else -1)
