milestone1:
	gcc dijkstra.c -o dijkstra

milestone2:
	gcc gui.c -o sim -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

milestone3:
	gcc gui.c -o sim -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
milestone4:
	gcc travelers.c -o sim -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
milestone5:
	gcc travelers.c -o sim -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
milestone6:
	gcc travelers.c -o sim -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
milestone7:
	gcc travelers.c -o sim -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

clean:
	rm -f dijkstra sim