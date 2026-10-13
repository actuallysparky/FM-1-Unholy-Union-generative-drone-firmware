import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('stage_public', Path(__file__).resolve().parents[1] / 'tools/stage_unholy_public_site.py')
stage = importlib.util.module_from_spec(spec)
spec.loader.exec_module(stage)


class PublicationTests(unittest.TestCase):
    def fixture(self, root):
        (root / 'web').mkdir()
        for name in stage.FILES:
            (root / 'web' / name).write_text('fixture')
        (root / 'web/unholy-installer.js').write_text("const experimental=channel.id==='experimental';\n")
        (root / 'web/unholy-release.js').write_text('export const firstTestRelease={enabled:true};\n')
        (root / 'LICENSES').mkdir()
        (root / 'LICENSE').write_text('license')
        (root / 'LICENSING.md').write_text('licensing')
        docs = root / 'docs'; docs.mkdir()
        for name in ('UnholyUnion-User-Manual-a3e780b.pdf', 'UnholyUnion-Quick-Start-a3e780b.pdf'):
            (docs / name).write_bytes(b'%PDF-fixture')
        return docs

    def test_public_copy_disables_private_query_and_metadata(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory); docs = self.fixture(root)
            source = root / 'build/validated/a3e780b'; source.mkdir(parents=True)
            (source / 'source').write_bytes(b'source')
            import hashlib
            pins = {'UnholyUnion-source-sdk-a3e780b.tar.gz': ('source', hashlib.sha256(b'source').hexdigest())}
            with patch.object(stage, 'ROOT', root), patch.object(stage, 'PINS', pins):
                stage.stage(root / 'public', docs)
            self.assertNotIn('first-test', (root / 'public/unholy-installer.js').read_text())
            self.assertIn('enabled:false', (root / 'public/unholy-release.js').read_text())
            self.assertTrue((root / 'public/index.html').is_file())
            self.assertTrue((root / 'web/UnholyUnion-Quick-Start-a3e780b.pdf').is_file())

    def test_unreviewed_existing_file_is_not_published(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory); docs = self.fixture(root)
            output = root / 'public'; output.mkdir(); (output / 'private.txt').write_text('private')
            with patch.object(stage, 'ROOT', root), patch.object(stage, 'PINS', {}):
                with self.assertRaisesRegex(ValueError, 'Unreviewed'):
                    stage.stage(output, docs)
            self.assertFalse((output / 'index.html').exists())


if __name__ == '__main__':
    unittest.main()
