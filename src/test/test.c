#include <stdio.h>
#include <soulstream.h>

int main() {
	const SoulstreamConfig config = {
		.username = "soulstream",
		.password = "password"
	};

	ss_init(config);

	ss_search("test");

	while(1) {
		ss_update();
	}

	ss_close();
}
