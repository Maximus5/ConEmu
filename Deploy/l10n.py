#!/usr/bin/python3
import argparse
import json
import os
import pprint
import requests
import sys

import yaml  # PyYAML | PyYAML.Yandex

from collections import OrderedDict
from enum import Enum
from requests.auth import HTTPBasicAuth


def parse_args():
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=
        'Usage examples:\n'
        'Create new language and pull it from Transifex (TX_TOKEN env var should be defined):\n'
        '  Deploy/l10n.py --l10n Release/ConEmu/ConEmu.l10n --add-lng es Español --tx-pull es --write-l10n\n')
    parser.add_argument('--l10n', required=True,
                        help='Filepath to ConEmu.l10n')
    parser.add_argument('--add-lng', nargs=2, action='append',
                        metavar=('ID', 'NAME'),
                        help='Create new language with <ID> and <NAME>')
    parser.add_argument('--lng-l10n', nargs=2, action='append',
                        metavar=('LNG', 'L10N'),
                        help='Append <LNG> data from <L10N> file')
    parser.add_argument('--tx-pull', action='append', metavar='LNG',
                        help='Pull <LNG> translation from Transifex; '
                             'use <all> for all translations')
    parser.add_argument('--tx-push', action='append', metavar='LNG',
                        help='Push <LNG> translation to Transifex')
    parser.add_argument('--write-l10n', action='store_true',
                        help='Update contents in <L10N> file')
    parser.add_argument('--write-yaml', metavar='DIR',
                        help='Write yaml resources to <DIR> folder')
    parser.add_argument('-v', '--verbose', action='store_true')
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
    def _set_str(resource, lng_id, item):
        deprecated = lng_id.startswith('_')
        set_lng_id = lng_id[1:] if deprecated else lng_id
        resource[set_lng_id] = {
                'item': LangData._get_str(item), 'deprecated': deprecated}

    def add_language(self, lng_id, lng_name):
        if lng_id is None or lng_id == '':
            raise Exception('lng_id is empty', lng_id)
        if lng_name is None or lng_name == '':
            raise Exception('lng_name is empty', lng_name)
        if lng_id in self.languages:
            if lng_name != self.languages[lng_id]:
                print('Language name changed, id={}, old_name={}'
                      ', new_name={}'.format(
                      lng_id, self.languages[lng_id], lng_name))
                self.languages[lng_id] = lng_name
            else:
                print('Language exists, id={}, name={}'.format(
                    lng_id, self.languages[lng_id]))
        else:
            print('New language, id={}, name={}'.format(
                  lng_id, lng_name))
            self.languages.setdefault(lng_id, lng_name)

    def _add_languages(self, new_langs, selected_lang=''):
        for lang in new_langs:
            if selected_lang == '' or selected_lang == lang['id']:
                self.add_language(lang['id'], lang['name'])
        return

    def get_translation_lang_ids(self, tx_langs=[]):
        if len(tx_langs) != 0 and not 'all' in tx_langs:
            return list(filter(lambda x: x != 'en', tx_langs))
        return list(filter(lambda x: x != 'en',
                    map(lambda x: x, self.languages.keys())))

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
            self._add_languages(l10n['languages'], selected_lang)
            print('Languages:', [id for id in self.languages])
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

    def update_lang(self, lng_id, data, verbose):
        if not lng_id in self.languages:
            raise Exception("Unknown lang_id={}".format(lng_id))
        for block_id in data:
            if not block_id in self.blocks:
                raise Exception("Block is not defined in the base", block_id)
            our_block = self.blocks[block_id]
            new_block = data[block_id]
            for str_id in new_block:
                if not str_id in our_block:
                    print("warning: str_id={} is not defined "
                          "in the base".format(str_id))
                    continue
                if new_block[str_id].strip() == '':
                    continue
                resource = our_block[str_id]
                new_str = new_block[str_id]
                if not lng_id in resource:
                    if verbose:
                        print('  new string[{}]: {}'.format(lng_id, new_str))
                    resource.setdefault(
                        lng_id,
                        {'item': new_str, 'deprecated': False})
                elif resource[lng_id]['item'] != new_str:
                    if verbose:
                        print('  changed string[{}]: "{}" -> "{}"'.format(
                            lng_id, resource[lng_id]['item'], new_str))
                    resource[lng_id]['item'] = new_str
                    resource[lng_id]['deprecated'] = False
        return

    @staticmethod
    def escape(line):
        return line.replace('\\', '\\\\').replace('"', '\\"').replace(
            '\n', '\\n').replace('\r', '\\r')

    def write_l10n(self, file):
        endl = '\n'

        def write_languages(file, indent):
            def write_language(file, lng_id, name, indent):
                file.write(
                    indent +
                    '{"id": "' + self.escape(lng_id) + '",' +
                    ' "name": "' + self.escape(self.languages[lng_id]) + '" }' +
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
                    return '"' + self.escape(item) + '"'
                data = '['
                is_first = True
                for line in item.splitlines(keepends=True):
                    if is_first:
                        is_first = False
                    else:
                        data += endl + indent + '      ,'
                    data += ' "'
                    data += self.escape(line)
                    data += '"'
                data = data + ' ]'
                return data

            def write_resource(file, lng_id, resource, indent):
                if lng_id == 'id':
                    return
                id = lng_id if not resource['deprecated'] else '_' + lng_id
                file.write(
                    indent + '"' + self.escape(id) + '": ' +
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
                        indent + '"' + self.escape(str_id) + '": {' +
                        endl)
                    rsrc = block[str_id]
                    for lng_id in self.languages:
                        if lng_id not in rsrc:
                            continue
                        write_resource(
                            file, lng_id, rsrc[lng_id], indent + '  ')
                    file.write(
                        indent + '  "id": ' + str(rsrc['id']) + ' }' +
                        endl)
                return

            for block_id in self.blocks:
                file.write(indent + ',' + endl)
                file.write(indent + '"' + self.escape(block_id) + '": {' + endl)
                write_block(file, self.blocks[block_id], indent + '  ')
                file.write(indent + '}' + endl)
                file.write('')
            return

        file.write('{' + endl)
        write_languages(file, '  ')
        write_blocks(file, '  ')
        file.write('}' + endl)
        return

    def write_yaml(self, folder):
        print('Writing yamls to {}'.format(folder))
        for lng_id in self.languages:
            file_path = os.path.join(folder, 'ConEmu_{}.yaml'.format(lng_id))
            print('  {}'.format(file_path))
            with open(file_path, 'w', encoding='utf-8-sig') as file:
                self.write_yaml_file(file, lng_id)
        return

    def create_yaml_string(self, lng_id):
        from io import StringIO
        file_str = StringIO()
        self.write_yaml_file(file_str, lng_id)
        return file_str.getvalue()

    def write_yaml_file(self, file, lng_id):
        endl = '\n'
        for block_id in self.blocks:
            file.write(block_id + ':' + endl)
            block = self.blocks[block_id]
            for str_id in sorted(block, key=lambda s: s.lower()):
                resource = block[str_id]
                if lng_id in resource:
                    file.write('  ' + str_id + ': "' +
                               self.escape(resource[lng_id]['item']) +
                               '"' + endl)
        return


class Transifex:
    def __init__(self):
        self.tx_token = os.environ['TX_TOKEN']
        # curl -k -L --user api:%TX_TOKEN% -X GET https://www.transifex.com/api/2/project/conemu-sources/resource/conemu-en-yaml--daily/translation/%1/?file=YAML_GENERIC -o %2
        self.base_url = 'https://www.transifex.com/api/2/project/conemu-sources/resource/conemu-en-yaml--daily'
        self.file_format = 'file=YAML_GENERIC'
        return

    def pull(self, lang_id):
        print('Pulling language {} from Transifex'.format(lang_id))
        result = requests.get(
            '{}/translation/{}/?{}'.format(
                self.base_url, lang_id, self.file_format),
            auth=HTTPBasicAuth('api', self.tx_token))
        print(' TX result: %s' % result.status_code)
        if result.status_code == 200:
            # print(result.encoding)
            result.encoding = 'utf-8'
            data = yaml.load(result.text, Loader=yaml.SafeLoader)
            # pp = pprint.PrettyPrinter(indent=2)
            # pp.pprint(data)
            return data
        raise Exception("No Transifex data", result)

    def push_source_language(self, json_data):
        result = requests.put(
            '{}/content/{}/'.format(self.base_url, lang_id),
            auth=HTTPBasicAuth('api', self.tx_token),
            json=json_data)
        print(' TX result: %s' % result.status_code)
        if result.status_code == 200:
            return
        raise Exception("No Transifex data", result)

    def push_translation_data(self, lang_id, yaml_data):
        print('Pushing language {} to Transifex'.format(lang_id))
        result = requests.put(
            '{}/translation/{}/'.format(self.base_url, lang_id),
            auth=HTTPBasicAuth('api', self.tx_token),
            files={'file': ('ConEmu_%s.yaml' % lang_id, yaml_data)})
        print(' TX result: %s' % result.status_code)
        if result.status_code == 200:
            import_result = result.json()
            print('  added:   %s' % import_result['strings_added'])
            print('  updated: %s' % import_result['strings_updated'])
            print('  deleted: %s' % import_result['strings_delete'])
            return
        raise Exception("No Transifex data", result, result.text)



def main(args):
    print("args", args)
    l10n = LangData()
    l10n.load_l10n_file(file_name=args.l10n)

    if args.lng_l10n is not None:
        for lng_pair in args.lng_l10n:
            l10n.load_l10n_file(selected_lang=lng_pair[0],
                                file_name=lng_pair[1])
    if args.add_lng is not None:
        for lng_pair in args.add_lng:
            l10n.add_language(lng_pair[0], lng_pair[1])
    if args.tx_pull is not None:
        tx = Transifex()
        for lng_id in l10n.get_translation_lang_ids(args.tx_pull):
            l10n.update_lang(lng_id, tx.pull(lng_id), args.verbose)
    if args.tx_push is not None:
        tx = Transifex()
        for lng_id in l10n.get_translation_lang_ids(args.tx_push):
            tx.push_translation_data(lng_id, l10n.create_yaml_string(lng_id))
    if args.write_l10n:
        with open(args.l10n, 'w', encoding='utf-8-sig') as l10n_file:
            print('Writing l10n to {}'.format(args.l10n))
            l10n.write_l10n(l10n_file)
    if args.write_yaml:
        l10n.write_yaml(args.write_yaml)
    return


if __name__ == "__main__":
    args = parse_args()
    print(args)
    main(args)

