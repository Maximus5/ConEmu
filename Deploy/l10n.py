#!/usr/bin/python3
import argparse
import json
import os
import requests
import sys

from collections import OrderedDict
from enum import Enum
from requests.auth import HTTPBasicAuth


class WorkMode(Enum):
    rc_to_l10n = 'rc-to-l10n'
    l10n_to_l10n = 'l10n-to-l10n'
    l10n_to_yaml = 'l10n-to-yaml'
    yaml_to_l10n = 'yaml-to-l10n'
    tx_pull = 'tx-pull'
    tx_push = 'tx-push'

    def __str__(self):
        return self.value


def parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument('--mode', default='rc-to-l10n',
                        type=WorkMode, choices=list(WorkMode))
    parser.add_argument('--l10n',
                        help='Filepath to ConEmu.l10n')
    parser.add_argument('--lng-l10n', nargs=2,
                        help='Append <LNG> data from <L10N> file')
    return parser.parse_args()


class LangData:
    def __init__(self):
        self.languages = OrderedDict([('en', 'English')])
        self.blocks = OrderedDict()
        print(self.languages)
        return

    @staticmethod
    def _get_str(item):
        if type(item) is str:
            return item
        if type(item) is list:
            return ''.join(item)
        raise Exception("Unknown type={}".format(type(item)))

    @staticmethod
    def _set_str(block, lng_id, item):
        deprecated = lng_id.startswith('_')
        id = lng_id[1:] if deprecated else lng_id
        block[id] = {'item': LangData._get_str(item), 'deprecated': deprecated}

    def _add_languages(self, new_langs, selected_lang=''):
        for lang in new_langs:
            if selected_lang == '' or selected_lang == lang['id']:
                self.languages[lang['id']] = lang['name']
        return

    def _add_block(self, name, data, selected_lang=''):
        our_block = self.blocks.setdefault(name, OrderedDict())
        for str_id in data:
            if selected_lang != '' and not str_id in our_block:
                raise Exception("Resource is not defined in the base",
                    str_id, data[str_id])
            our_str = our_block.setdefault(str_id, {})
            for lng_id in data[str_id]:
                if lng_id == '':
                    print("error: bad id:", data[str_id])
                    continue
                if lng_id == 'id':
                    if selected_lang == '':
                        our_str['id'] = data[str_id]['id']
                    continue
                if selected_lang == '' or lng_id == selected_lang:
                    LangData._set_str(our_str, lng_id, data[str_id][lng_id])
        # print("\n_add_block:", name, " = ", our_block)
        return

    def load_l10n_file(self, file_name, selected_lang=''):
        print("Loading l10n file: {}, language: {}".format(
              file_name, selected_lang if selected_lang != '' else '*'))
        with open(file_name, 'r', encoding='utf-8-sig') as l10n_file:
            l10n = json.load(l10n_file, object_pairs_hook=OrderedDict)
            # Copy language descriptions to our dict
            print('langs before:', [id for id in self.languages])
            self._add_languages(l10n['languages'], selected_lang)
            print('langs after:', [id for id in self.languages])
            # Copy string blocks to our dict
            total_count = 0
            for block_id in l10n:
                # Languages are processed separately
                if block_id == 'languages':
                    continue
                print('Block {}'.format(block_id), end='')
                self._add_block(block_id, l10n[block_id], selected_lang)
                count = len(self.blocks[block_id])
                total_count += count
                print("\tcount={}".format(count))
            print("Total resources count={}".format(total_count))
        return

    def write_l10n(self, file):
        endl = '\n'

        def escape(line):
            return line.replace('\\', '\\\\').replace('"', '\\"').replace(
                '\n', '\\n').replace('\r', '\\r')

        def write_languages(file, indent):
            def write_language(file, lng_id, name, indent):
                file.write(
                    indent +
                    '{"id": "' + escape(lng_id) + '",' +
                    ' "name": "' + escape(self.languages[lng_id]) + '" }' +
                    endl)

            file.write(indent + '"languages": [' + endl)
            is_first = True
            for lng_id in self.languages:
                if is_first:
                    is_first = False
                else:
                    file.write(indent + '  ,' + endl)
                write_language(
                    file, lng_id, self.languages[lng_id],
                    indent + '  ')
            file.write(indent + ']' + endl)
            return

        def write_blocks(file, indent):
            def make_string(item, indent):
                if not '\n' in item:
                    return '"' + escape(item) + '"'
                data = '['
                is_first = True
                for line in item.splitlines(keepends=True):
                    if is_first:
                        is_first = False
                    else:
                        data += endl + indent + '      ,'
                    data += ' "'
                    data += escape(line)
                    data += '"'
                data = data + ' ]'
                return data

            def write_resource(file, lng_id, resource, indent):
                if lng_id == 'id':
                    return
                id = lng_id if not resource['deprecated'] else '_' + lng_id
                file.write(
                    indent + '"' + escape(id) + '": ' +
                    make_string(resource['item'], indent) +
                    ',' + endl)
                return

            # Write language block, e.g. "cmnhints"
            def write_block(file, block, indent):
                is_first = True
                for str_id in sorted(block, key=lambda s: s.lower()):
                    if is_first:
                        is_first = False
                    else:
                        file.write(indent + ',' + endl)
                    file.write(
                        indent + '"' + escape(str_id) + '": {' +
                        endl)
                    rsrc = block[str_id]
                    for lng_id in rsrc:
                        write_resource(
                            file, lng_id, rsrc[lng_id], indent + '  ')
                    file.write(
                        indent + '  "id": ' + str(rsrc['id']) + ' }' +
                        endl)
                return

            for block_id in self.blocks:
                file.write(indent + ',' + endl)
                file.write(indent + '"' + escape(block_id) + '": {' + endl)
                write_block(file, self.blocks[block_id], indent + '  ')
                file.write(indent + '}' + endl)
                file.write('')
            return

        file.write('{' + endl)
        write_languages(file, '  ')
        write_blocks(file, '  ')
        file.write('}' + endl)
        pass


class Transifex:
    def __init__(self):
        self.tx_token = os.environ['TX_TOKEN']
        # curl -k -L --user api:%TX_TOKEN% -X GET https://www.transifex.com/api/2/project/conemu-sources/resource/conemu-en-yaml--daily/translation/%1/?file=YAML_GENERIC -o %2
        self.base_url = 'https://www.transifex.com/api/2/project/conemu-sources/resource/conemu-en-yaml--daily'
        self.file_format = 'file=YAML_GENERIC'
        return

    def pull(self, lang_id):
        result = requests.get(
            '{}/translation/{}/?{}'.format(
                self.base_url, lang_id, self.file_format),
            auth=HTTPBasicAuth('api', self.tx_token))
        print(result)
        if result.status_code == 200:
            print(result.text)
        return


def main(args):
    print("args", args)
    l10n = LangData()
    if not args.l10n is None:
        l10n.load_l10n_file(file_name=args.l10n)
    if not args.lng_l10n is None:
        l10n.load_l10n_file(file_name=args.lng_l10n[1],
                            selected_lang=args.lng_l10n[0])
    if args.mode == WorkMode.l10n_to_l10n:
        # l10n.write_l10n(sys.stdout)
        with open(args.l10n, 'w', encoding='utf-8-sig') as l10n_file:
            l10n.write_l10n(l10n_file)
        # for blk in l10n.blocks:
        #     print(blk, ":")
        #     for res in l10n.blocks[blk]:
        #         print("  ", res, " = ", l10n.blocks[blk][res])
    elif args.mode == WorkMode.tx_pull:
        tx = Transifex()
        tx.pull('es')
    else:
        raise Exception("Unsupported mode", args.mode)

    return


if __name__ == "__main__":
    args = parse_args()
    main(args)
