# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# def options(opt):
#     pass

# def configure(conf):
#     conf.check_nonfatal(header_name='stdint.h', define_name='HAVE_STDINT_H')

def build(bld):
    module = bld.create_ns3_module('ns3asy', ['core'])
    module.source = [
        'model/ns3asy.cc',
        'model/genericApp.cc',
        'model/defaultCallbacks.cc',
        'helper/ns3asy-helper.cc',
        ]

    module_test = bld.create_ns3_module_test_library('ns3asy')
    module_test.source = [
        'test/ns3asy-test-suite.cc',
        ]

    headers = bld(features='ns3header')
    headers.module = 'ns3asy'
    headers.source = [
        'model/ns3asy.h',
        'model/genericApp.h',
        'model/defaultCallbacks.h',
        'helper/ns3asy-helper.h',
        ]

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    # bld.ns3_python_bindings()

