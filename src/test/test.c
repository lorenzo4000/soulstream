#include <stdio.h>
#include <soulstream.h>

int main() {
	printf("Hello, World!\n");

	const SoulstreamConfig config = {
		.username = "soulstream",
		.password = "password"
	};

	soulstream_init(config);

	search("test");

	while(1) {
		soulstream_update();
	}

	soulstream_close();
}
