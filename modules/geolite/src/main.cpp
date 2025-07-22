#include <iostream>

#include <argparse/argparse.hpp>
#include <unirec++/unirec.hpp>

#define DEFAULT_DIRECTION true

int main(int argc, char** argv)
{
	argparse::ArgumentParser program("Geolite");

	Nemea::Unirec unirec({1, 1, "geolite", "Geolite module"});

	try {
		unirec.init(argc, argv);
	} catch (const Nemea::HelpException& ex) {
		std::cerr << program;
		return EXIT_SUCCESS;
	}

	// TODO: make it load both ip directions

	try {
		program.add_argument("-d", "--direction")
			.help("If specified both IP directions used for geolocation")
			.default_value(DEFAULT_DIRECTION)
			.implicit_value(!DEFAULT_DIRECTION);
		program.add_argument("-f", "--field")
			.required()
			.help("Name of Unirec field with IP address")
			.default_value(std::string("SRC_IP"));
		program.add_argument("-p", "--path")
			.help("Specifiy the path to database files")
			.default_value(std::string("~/GeoLite2-City_20250718/GeoLite2-City.mmdb"));
		program.parse_args(argc, argv);
	} catch (const std::exception& ex) {
		std::cerr << program;
		return EXIT_FAILURE;
	}

	auto direction = program.get<bool>("--direction");
	auto field = program.get<std::string>("--field");

	std::cout << "--field: " << field << '\n';
	std::cout << "--direction: " << direction << '\n';

	return 0;
}
