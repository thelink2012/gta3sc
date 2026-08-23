import lit.formats


class GTA3ScriptTest(lit.formats.ShTest):
    """Only discovers tests directly under the suite directory.

    This means e.g. codegen/file.sc is a test, but codegen/multifile/gosub1.sc is not.
    The latter is meant to be a support script pulled by a test script.
    """

    def getTestsInDirectory(self, testSuite, path_in_suite, litConfig,
                            localConfig):
        # Only allow a single level of recursion below the test root.
        if len(path_in_suite) == 1:
            yield from super().getTestsInDirectory(
                testSuite, path_in_suite, litConfig, localConfig)
