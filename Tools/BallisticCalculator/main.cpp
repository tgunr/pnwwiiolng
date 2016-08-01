#include "weapammo.h"

#include <cstdio>
#include <exception>


// First param is the ammo file to read, and is always required.
// Second param is how fine to run the calculations, in milliseconds. Defaults to 60 FPS.
// Third param is how far out to simulate in meters. Defaults to 3km.
int main(int argc, char* argv[]) {
	FILE* out = nullptr;

	try {
		ReadBallisticChart(); // Need to init stuff.

		if ( argc < 2 )
			throw std::runtime_error("The ammo file should be passed in as the first argument.");
		weapAmmoType ammo_type = ReadAmmoType(argv[1]);
		weapAmmo round = CreateRound(ammo_type);

		int max_dist = 3000;
		if ( argc >= 4 )
			max_dist = std::stoi(argv[2]);

		double step = 1.0f / 60.0f;
		if ( argc >= 3 )
			step = std::stoi(argv[3]) / 1000.0;

		std::string out_filename(std::string("ballistics/") + argv[1] + ".txt");
		out = fopen(out_filename.c_str(), "w");
		if ( ! out )
			throw std::runtime_error("Failed to create '" + out_filename + ".txt'");

		float current_time = 0.0f;
		while ( mathExactVectorLength(&round.Position) < max_dist ) {
			if ( current_time > 2 ) // TESTING
				break;

			UpdateProjectile(&round, (current_time += step));
			double distance = mathExactVectorLength(&round.Position);
			double pierces = PiercingPotential(round);
			fprintf(out, "%f sec : %f meters. Can pierce %f mm\n",
			        current_time, distance, pierces);
		}
		fclose(out);
	} catch ( std::exception& e ) {
		if ( out ) {
			fprintf(out, "Error: %s", e.what());
			fclose(out);
		}

		fprintf(stderr, "Error: '%s'", e.what());
		return -1;
	}

	return 0;
}