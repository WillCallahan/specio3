import os
import unittest
import specio3
from pathlib import Path

class SpcFileTests(unittest.TestCase):
    def setUp(self):
        self.test_path = Path(__file__).parent.absolute()
        self.data_path = os.path.join(self.test_path, 'data')

    def _test_read_multifile(self, file):
        print(f"Testing file: {file}")
        test_data = os.path.join(self.data_path, file)
        data = specio3.read_spc(test_data)
        self.assertGreater(len(data), 0)
        for item in data:
            self.assertEqual(len(item), 2)
            self.assertEqual(item[0].shape, item[1].shape)

    def test_read_multifile(self):
        # Only test .spc files, filter out system files like .DS_Store
        files = [f for f in os.listdir(self.data_path) if f.lower().endswith('.spc')]
        files.sort()  # Sort for consistent test order
        
        for file in files:
            self._test_read_multifile(file)


if __name__ == '__main__':
    unittest.main()