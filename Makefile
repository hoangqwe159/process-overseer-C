
overseer=overseer.c helpers.c
controller=controller.c helpers.c
exec= exec_cmd.c helpers.c

# Fix the directories to match your file organisation.
CC_FLAGS=-std=gnu99 -Wall -g

all: overseer controller exec

overseer: 
	gcc $(CC_FLAGS) $(overseer) -lpthread -I. -o $@

controller:
	gcc $(CC_FLAGS) $(controller) -I. -o $@

exec:
	gcc $(CC_FLAGS) $(exec) -I. -o $@

.PHONY: clean
clean:
	@rm -f $(OBJ) *.o *.exe overseer controller exec