final: userio recv send
	@gcc brain.c -o system
	@echo "Run ./system <portnumber>"

userio:userio.c
	@gcc userio.c -o userio

recv:ear.c
	@gcc ear.c -o recv

send:mouth.c
	@gcc mouth.c -o send

clean:
	@rm userio recv send system *btm.txt *etb.txt *utb.txt *btu.txt
