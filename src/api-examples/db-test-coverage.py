#!/usr/bin/env python
import json
import sys
import subprocess
import tempfile
from lxml import etree
from StringIO import StringIO

from common import get_url

def run_gccxml(path, includes):
    tempFile = tempfile.NamedTemporaryFile('r')
    cmdArray = ['gccxml', path, '-fxml=' + tempFile.name]
    for i in includes:
        cmdArray += ['-I', i]
    proc = subprocess.Popen(cmdArray, stdin=None, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    code = proc.wait()
    if code != 0:
        print proc.stderr.read()
        raise Exception('Could not parse ' + path)
    return tempFile.read()


def get_file_id(xml, path):
    matching = xml.xpath("File[@name='%s']" % path)
    if len(matching) == 0:
        raise Exception('Source file not found in the intermediate xml')
    fileNode = matching[0]
    return fileNode.get('id')


def get_klass_id(xml, file_id, klass):
    matching = xml.xpath("Class[@name='%s' and @file='%s']" % (klass, file_id))
    if len(matching) == 0:
        raise Exception("Can not find the class %s::%s in the intermediate xml" % (file_id, klass))
    klassNode = matching[0]
    return klassNode.get('id')


def get_klass_methods(xml, klass_id):
    methods = xml.xpath("Method[@context='%s' and @access='public']" % klass_id)
    return map(lambda m: m.get('name'), methods)


def get_functions(xml, file_id):
    functions = xml.xpath("Function[@file='%s']" % file_id)
    return map(lambda f: f.get('name'), functions)


def get_methods(xml, file_id, klass=None):
    if klass:
        klass_id = get_klass_id(xml, file_id, klass)
        return get_klass_methods(xml, klass_id)
    else:
        return get_functions(xml, file_id)


def get_db_ifce_methods():
    xml_raw = run_gccxml('../../db/generic/GenericDbIfce.h', ['../..', '../../common'])
    xml = etree.parse(StringIO(xml_raw))
    file_id = get_file_id(xml, '../../db/generic/GenericDbIfce.h')
    return get_methods(xml, file_id,  'GenericDbIfce')


def get_executed_methods(endpoint):
    data_raw = get_url(endpoint + 'stats/profiling', showall = 1)
    data = json.loads(data_raw)

    db_methods = filter(lambda p: p.startswith('DB::'), [p['scope'] for p in data['profiles']])
    return map(lambda m: str(m[4:]), db_methods)


def get_missing_methods(all, actual):
    return filter(lambda m: m not in actual, all)

if __name__ == '__main__':
    if len(sys.argv) > 1:
        endpoint = sys.argv[1]
    else:
        endpoint = 'https://fts3-devel.cern.ch:8449/fts3/ftsmon/'

    db_methods = get_db_ifce_methods()
    executed_methods = get_executed_methods(endpoint)

    missing_methods = get_missing_methods(db_methods, executed_methods)

    print "Methods not executed in the server (%d):" % (len(missing_methods))
    for m in sorted(missing_methods):
        print m

    ndb = len(db_methods)
    ne  = len(executed_methods)
    percent = (ne * 100.0) / ndb
    print "Coverage %d out of %d (%.2f%%)" % (ne, ndb, percent)
