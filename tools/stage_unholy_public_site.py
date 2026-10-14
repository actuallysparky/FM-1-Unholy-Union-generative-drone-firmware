#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Stage allowlisted public firmware channels. Does not deploy or access devices."""
import argparse
import hashlib
import json
import re
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FILES = ('unholy-installer.html', 'unholy-installer.css', 'unholy-installer.js',
         'unholy-controller.js', 'unholy-bundled-package.js', 'unholy-release.js', 'unholy-channels.js', 'unholy-catalog.js',
         'fm1ota.js', 'fm1pkg.js', 'fm1backup.js', 'holy-rnd.js', 'holy-rnd-guide.html',
         'unholy-inline-backup.js', 'unholy-holyrnd-revelation.svg', 'unholy-sigil.svg',
         'unholy-drone-preview.png', 'unholy-quick-start.html', 'editor.html',
         'fukiai.ttf', 'FUKIAI-LICENSE.txt', 'unholy-macros.html', 'unholy-macros.js')
PINS = {
    'unholyunion-permutation-0.2.3.fwsc': ('firmware/unholyunion-permutation-0.2.3.fwsc','025bec05d71da1b106ecb05d52e165ea6592812e92209cfe2626278047f7abba','permutation-0.2.3'),
    'UnholyUnion-source-sdk-permutation-0.2.3.tar.gz': ('UnholyUnion-source-sdk-permutation-0.2.3.tar.gz','7edb541249f0f01db5f6dc1d9ca21a123747faaca1f33bb624deaa57d68fde45','permutation-0.2.3'),
    'unholyunion-permutation-0.2.2.fwsc': ('firmware/unholyunion-permutation-0.2.2.fwsc','d6c5c1f74590ff28712f916eabfa7040b2cf606a77914058c7d003a867fba3f8','permutation-0.2.2'),
    'UnholyUnion-source-sdk-permutation-0.2.2.tar.gz': ('UnholyUnion-source-sdk-permutation-0.2.2.tar.gz','45667a299f27eb2c5e7342d8803b46d23454e840e38e475477c37fbee0314cd0','permutation-0.2.2'),
    'unholyunion-permutation-0.2.1.fwsc': ('firmware/unholyunion-permutation-0.2.1.fwsc','498606000c4f63db3d846d537011d1afe11f36d1c3ede9ee072428804bd1133b','permutation-0.2.1'),
    'UnholyUnion-source-sdk-permutation-0.2.1.tar.gz': ('UnholyUnion-source-sdk-permutation-0.2.1.tar.gz','9e7f645181c369c51aebc6167a0a8360ba561cef242f34632efa9edcfe86e934','permutation-0.2.1'),
    'unholyunion-permutation-0.2.0.fwsc': ('firmware/unholyunion-permutation-0.2.0.fwsc','82067a3f3c58e76a4af09fa0e2b16edff40cf30ff577d53f355cd0e347588ef5','permutation-0.2.0'),
    'UnholyUnion-source-sdk-permutation-0.2.0.tar.gz': ('UnholyUnion-source-sdk-permutation-0.2.0.tar.gz','aefb72114b5d50a5b65dc3b750ce9390a475c96d20c1f162a9c6ff3b6ca12543','permutation-0.2.0'),
    'unholyunion-permutation-0.1.9.fwsc': ('firmware/unholyunion-permutation-0.1.9.fwsc','909a80c7db95f1a602c94d85d96ec61f328d18e6eec5733cb8a3bdaa75406ab9','permutation-0.1.9'),
    'UnholyUnion-source-sdk-permutation-0.1.9.tar.gz': ('UnholyUnion-source-sdk-permutation-0.1.9.tar.gz','9b1ac420f8f285c550a46a0ab2d7fef6931cf32409211e992d16120ac33eec4c','permutation-0.1.9'),
    'unholyunion-permutation-0.1.8.fwsc': ('firmware/unholyunion-permutation-0.1.8.fwsc','b60082dc595fe076eaf8d0f03025ca397b3413c8380962578f42267235a5eb01','permutation-0.1.8'),
    'UnholyUnion-source-sdk-permutation-0.1.8.tar.gz': ('UnholyUnion-source-sdk-permutation-0.1.8.tar.gz','ee6544e6e5a59181bfa58160a70ccf4ca6dcba6e3aaf8d1586be4fe0803887b4','permutation-0.1.8'),
    'unholyunion-permutation-0.1.7.fwsc': ('firmware/unholyunion-permutation-0.1.7.fwsc','fb746436f4c0817c165b873569523c9457cb70a1db50961d1b7bb2dfa2bfbc3d','permutation-0.1.7'),
    'UnholyUnion-source-sdk-permutation-0.1.7.tar.gz': ('UnholyUnion-source-sdk-permutation-0.1.7.tar.gz','89ebff6fec50ed55e025c924c6bfc06d1216a06bc3ec89266aa58492d501da6d','permutation-0.1.7'),
    'unholyunion-permutation-0.1.6.fwsc': ('firmware/unholyunion-permutation-0.1.6.fwsc','448775126b000d0840c78a110e1937bb92f5e7387da4203d79cffe427f71bc07','permutation-0.1.6'),
    'UnholyUnion-source-sdk-permutation-0.1.6.tar.gz': ('UnholyUnion-source-sdk-permutation-0.1.6.tar.gz','45aede1aff2ec80ffee0233d243755fec43bede83dbd6db011811342283bc846','permutation-0.1.6'),
    'unholyunion-permutation-0.1.5.fwsc': ('firmware/unholyunion-permutation-0.1.5.fwsc','c2965f2e97c279489889b3c46fd66039bed1c84879ccff55c1053d445ad7a0df','permutation-0.1.5'),
    'UnholyUnion-source-sdk-permutation-0.1.5.tar.gz': ('UnholyUnion-source-sdk-permutation-0.1.5.tar.gz','1cfc38b5539bd0e5066a7c942e2725cd65ec001210c0e21ca4cf36eb2975444a','permutation-0.1.5'),
    'unholyunion-permutation-0.1.4.fwsc': ('firmware/unholyunion-permutation-0.1.4.fwsc','a8aa587f71f45058ef5a11ddf571d7674b059df63a70a4a43516eb9d4ced1d44','permutation-0.1.4'),
    'UnholyUnion-source-sdk-permutation-0.1.4.tar.gz': ('UnholyUnion-source-sdk-permutation-0.1.4.tar.gz','3140fc0ad983e35402fcf3de545c97256d2a9caa2fded32e26d46101797e9bb1','permutation-0.1.4'),
    'unholyunion-permutation-0.1.3.fwsc': ('firmware/unholyunion-permutation-0.1.3.fwsc','f16ec0b46d1f56f2ba30e5d39f71f13c84f1cf57af96ad80ba42d8745058f264','permutation-0.1.3'),
    'UnholyUnion-source-sdk-permutation-0.1.3.tar.gz': ('UnholyUnion-source-sdk-permutation-0.1.3.tar.gz','921581b60f8ae056d4574cc59e21481d81e742a320ea41bc1ebfa1881e968c03','permutation-0.1.3'),
    'unholyunion-permutation-0.1.2.fwsc': ('firmware/unholyunion-permutation-0.1.2.fwsc','027e234f4f5f37be4a6d94ea0841ae8f7b0a218bc043dd27ec83df0000451f5a','permutation-0.1.2'),
    'UnholyUnion-source-sdk-permutation-0.1.2.tar.gz': ('UnholyUnion-source-sdk-permutation-0.1.2.tar.gz','2e73efb1a6cf591a869a9d4df17186321c3d11a919daf1492c0b801efade5e8a','permutation-0.1.2'),
    'unholyunion-holy-rnd-0.1.1.fwsc': ('firmware/unholyunion-holy-rnd-0.1.1.fwsc','8fb6fd00f8e891472d0947ab0e4625cfdd5cd381b31a34958af18238f72a0d9c','holy-rnd-0.1.1'),
    'UnholyUnion-source-sdk-holy-rnd-0.1.1.tar.gz': ('UnholyUnion-source-sdk-holy-rnd-0.1.1.tar.gz','ea81f0d1e86b384d268b40f3293022f186097068023d1e85a0ec4d3cdc973d5c','holy-rnd-0.1.1'),
    'unholyunion-a3e780b.fwsc': ('firmware/unholyunion-experimental.fwsc',
        '0f0685775ba9e0a00b1bfb03c1a3c70b188a712d0752689d0a63ae93cb38f517'),
    'UnholyUnion-source-sdk-a3e780b.tar.gz': ('UnholyUnion-source-sdk-a3e780b.tar.gz',
        '9b0a425ba61e71cf1e0ab9436a4c254d079d65efe1d4249c307c60435b4a742e'),
}

def stage(destination, docs):
    # Validate all inputs before touching the destination.
    inputs = {name: (ROOT / 'web' / name).read_bytes() for name in FILES}
    for name, pin in PINS.items():
        source, digest = pin[:2]
        build = pin[2] if len(pin)>2 else 'a3e780b'
        data = (ROOT / 'build/validated' / build / source).read_bytes()
        if hashlib.sha256(data).hexdigest() != digest:
            raise ValueError(f'Pinned artifact differs: {name}')
        inputs[name] = data
    for name in ('UnholyUnion-Quick-Start-0.2.3.pdf', 'UnholyUnion-Quick-Start-0.2.3-source.zip',
                 'UnholyUnion-User-Manual-0.2.3.pdf', 'UnholyUnion-Manual-0.2.3-source.zip',
                 'UnholyUnion-User-Manual-0.2.2.pdf', 'UnholyUnion-Manual-0.2.2-source.zip',
                 'UnholyUnion-User-Manual-0.2.1.pdf', 'UnholyUnion-Manual-0.2.1-source.zip',
                 'UnholyUnion-User-Manual-0.2.0.pdf', 'UnholyUnion-Manual-0.2.0-source.zip',
                 'UnholyUnion-User-Manual-0.1.9.pdf', 'UnholyUnion-Manual-0.1.9-source.zip',
                 'UnholyUnion-User-Manual-0.1.8.pdf', 'UnholyUnion-Manual-0.1.8-source.zip',
                 'UnholyUnion-User-Manual-0.1.7.pdf', 'UnholyUnion-Manual-0.1.7-source.zip',
                 'UnholyUnion-User-Manual-0.1.6.pdf', 'UnholyUnion-Manual-0.1.6-source.zip',
                 'UnholyUnion-User-Manual-0.1.5.pdf', 'UnholyUnion-Manual-0.1.5-source.zip',
                 'UnholyUnion-User-Manual-0.1.4.pdf', 'UnholyUnion-Manual-0.1.4-source.zip',
                 'UnholyUnion-User-Manual-a3e780b.pdf', 'UnholyUnion-Quick-Start-a3e780b.pdf'):
        inputs[name] = (docs / name).read_bytes()
    for name in ('LICENSE', 'LICENSING.md'):
        inputs[name] = (ROOT / name).read_bytes()
    for path in (ROOT / 'LICENSES').iterdir():
        if path.is_file():
            inputs['LICENSES/' + path.name] = path.read_bytes()
    revision = hashlib.sha256(Path(__file__).read_bytes() + b''.join(inputs[name] for name in FILES)).hexdigest()[:12]
    destination.mkdir(parents=True, exist_ok=True)
    if destination.is_symlink() or any(p.is_symlink() for p in destination.rglob('*')):
        raise ValueError('Symlinks are not publication inputs')
    expected = set(inputs) | {'index.html', 'publication-manifest.json'}
    unexpected = {str(p.relative_to(destination)) for p in destination.rglob('*') if p.is_file()} - expected
    if unexpected:
        raise ValueError(f'Unreviewed existing publication files: {sorted(unexpected)}')
    for name, data in inputs.items():
        if name.endswith(('.html', '.js', '.css')):
            text = data.decode()
            text = re.sub(r'\?v=[\w.-]+', '?v=hosted-' + revision, text)
            if name == 'unholy-installer.html':
                text = re.sub(r'<div id="experiment-banner".*?</div>', '', text, flags=re.S)
                text = text.replace('PRIVATE BUILD VERIFIED', 'EXPERIMENTAL BUILD VERIFIED')
                text = text.replace('The private a3e780b build', 'The experimental a3e780b build')
                text = text.replace(' The separately approved first-test option does not claim hardware validation.', '')
                text = text.replace('Sparky has accepted losing the FM-1 user presets and patterns for this experimental test. Their backup is not a prerequisite for this session.', 'Back up your user presets and patterns before any future approved update. New-format projects cannot be read by older firmware.')
                text = text.replace(' The approved first experiment accepts that independent recovery is not prepared or guaranteed.', ' Recovery has not been independently validated.')
                text = text.replace('I accept loss of FM-1 user presets/patterns and have retained official recovery firmware.', 'I have backed up my data and retained official recovery firmware.')
                text = text.replace('UNHOLYUNION / PRIVATE LAB', 'UNHOLYUNION / EXPERIMENTAL')
            if name == 'unholy-release.js':
                text = re.sub(r'^(export const firstTestRelease=.*)$', lambda m: m[0].replace('enabled:true', 'enabled:false'), text, flags=re.M)
                text = re.sub(r'// Sparky approved.*', '// Public hosting never enables the private first-test authorization.', text)
            if name == 'unholy-quick-start.html':
                text = text.replace('No external hosting was created.', 'The installer and downloadable guides are hosted at https://citadeloftruth.duckdns.org/local/unholyunion/.')
                text = text.replace('Private local guide', 'Source-verified guide')
            data = text.encode()
        (destination / name).parent.mkdir(parents=True, exist_ok=True)
        (destination / name).write_bytes(data)
    # Same page at the requested directory entry point and its existing filename.
    shutil.copyfile(destination / 'unholy-installer.html', destination / 'index.html')
    manifest = {str(p.relative_to(destination)): {'bytes': p.stat().st_size,
        'sha256': hashlib.sha256(p.read_bytes()).hexdigest()}
        for p in sorted(destination.rglob('*')) if p.is_file() and p.name != 'publication-manifest.json'}
    (destination / 'publication-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    # Keep the existing localhost preview's new download links functional too.
    for name in ('UnholyUnion-User-Manual-0.2.3.pdf', 'UnholyUnion-User-Manual-0.2.2.pdf',
 'UnholyUnion-User-Manual-0.2.1.pdf',
 'UnholyUnion-User-Manual-0.2.0.pdf',
 'UnholyUnion-User-Manual-0.1.9.pdf',
 'UnholyUnion-User-Manual-0.1.8.pdf', 'UnholyUnion-User-Manual-0.1.7.pdf', 'UnholyUnion-User-Manual-0.1.6.pdf', 'UnholyUnion-User-Manual-0.1.5.pdf', 'UnholyUnion-User-Manual-0.1.4.pdf',
                 'UnholyUnion-User-Manual-a3e780b.pdf', 'UnholyUnion-Quick-Start-a3e780b.pdf',
                 'UnholyUnion-source-sdk-a3e780b.tar.gz', 'LICENSE'):
        target = ROOT / 'web' / name
        if target.is_symlink():
            raise ValueError('Refusing a symlink preview download')
        target.write_bytes(inputs[name])
    print(f'Staged {len(manifest)} public files; channel pins and approval policies preserved.')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--destination', required=True, type=Path)
    parser.add_argument('--docs', required=True, type=Path, help='Canonical UnholyUnion docs directory')
    args = parser.parse_args()
    stage(args.destination, args.docs)
