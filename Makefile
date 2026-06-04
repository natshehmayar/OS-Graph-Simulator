milestone1:
	gcc dijkstra.c -o dijkstra

milestone2:
	gcc gui.c -o sim -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

milestone3:
	gcc gui.c -o sim -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
milestone4:
	gcc gui.c -o sim -lraylib -lGL -lm -lpthread -ldl -lrt -lX11


clean:
	rm -f dijkstra sim