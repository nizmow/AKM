"""Regression tests for the lexical CPX foreign-ABI lint."""
import unittest

from cpx_abi_lint import lint_text


class CpxAbiLintTests(unittest.TestCase):
    def assert_rejected(self, source: str, member: str) -> None:
        errors = lint_text(source)
        self.assertTrue(any(member in message for _, message in errors), errors)

    def test_direct_call_is_rejected(self):
        self.assert_rejected("result = xcpb->Xform_do(tree, 0, msg);", "Xform_do")

    def test_parenthesized_call_is_rejected(self):
        self.assert_rejected("result = (xcpb->Xform_do)(tree, 0, msg);", "Xform_do")

    def test_callback_alias_extraction_is_rejected(self):
        self.assert_rejected("fn = xcpb->Get_Buffer;", "Get_Buffer")

    def test_wrong_bridge_is_rejected(self):
        self.assert_rejected("cpx_call2((void *)xcpb->Xform_do, 0, 0);", "Xform_do")

    def test_callback_in_nonfirst_argument_is_rejected(self):
        self.assert_rejected("cpx_call2(0, (void *)xcpb->Get_Buffer, 0);", "Get_Buffer")

    def test_nested_extraction_does_not_bypass_bridge(self):
        self.assert_rejected(
            "cpx_call2((void *)consume(xcpb->Get_Buffer), 0, 0);", "Get_Buffer")

    def test_callback_value_cannot_be_transformed(self):
        self.assert_rejected(
            "cpx_call2((void *)(other, xcpb->Get_Buffer), 0, 0);", "Get_Buffer")

    def test_dot_and_indexed_receivers(self):
        self.assert_rejected("copy.Get_Buffer();", "Get_Buffer")
        self.assertEqual(
            lint_text("cpx_call2((void *)blocks[0]->Get_Buffer, 0, 0);"), [])

    def test_approved_wrappers_pass(self):
        source = """
            cpx_xform_do((void *)xcpb->Xform_do, tree, 0, msg);
            cpx_rsh_obfix((void *)(xcpb->rsh_obfix), tree, ob);
            cpx_xgen_alert((void *)xcpb->XGen_Alert, 0);
            cpx_call2((void *)xcpb->CPX_Save, vars, size);
            cpx_call2((void *)((xcpb->GetFirstRect)), rect, 0);
        """
        self.assertEqual(lint_text(source), [])

    def test_comments_and_quoted_literals_are_ignored(self):
        source = '''
            /* xcpb->Xform_do(); */
            // xcpb->rsh_obfix\n
            char *s = "xcpb->Get_Buffer()";
            char c = 'x';
        '''
        self.assertEqual(lint_text(source), [])

    def test_scalar_fields_are_not_callbacks(self):
        self.assertEqual(lint_text("short n = xcpb->Country_Code + xcpb->handle;"), [])

    def test_multiline_bridge_and_diagnostic_line(self):
        source = "cpx_xform_do(\n    (void *)xcpb->Xform_do,\n    tree, 0, msg);"
        self.assertEqual(lint_text(source), [])
        errors = lint_text("xcpb->\n    Popup(items, 0, 0, 0, a, b);")
        self.assertEqual(errors[0][0], 2)

    def test_unmapped_callbacks_are_forbidden(self):
        self.assert_rejected("save = xcpb->save_header;", "save_header")


if __name__ == "__main__":
    unittest.main()
