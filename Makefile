run: nicsh
	@./nicsh

nicsh: nicsh.cpp
	@g++ -o $@ $^

clean:
	@rm -rf nicsh
