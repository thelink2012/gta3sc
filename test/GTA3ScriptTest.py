import lit.formats

# Configuration file for the 'lit' test runner.

# Our own getTestsInDirectory to avoid too avoid reading subscripts. 
class GTA3ScriptTest(lit.formats.ShTest):
    def __init__(self, execute_external = False):
        super(GTA3ScriptTest, self).__init__(execute_external=execute_external)


    def getTestsInDirectory(self, testSuite, path_in_suite,
                            litConfig, localConfig):
        # Only allow one level of recursion in the test paths
        if len(path_in_suite) == 1:
            return super(GTA3ScriptTest, self).getTestsInDirectory(testSuite, path_in_suite, litConfig, localConfig)
        return []
