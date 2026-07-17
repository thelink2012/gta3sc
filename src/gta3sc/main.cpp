#include <gta3sc/cli/run.hpp>
#include <iostream>

int main(int argc, char** argv)
{
    return gta3sc::cli::run(argc, argv, std::cout, std::cerr);
}
