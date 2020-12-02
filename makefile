
wordplay : wordplay.c
	$(CC) $(CFLAGS) $(LDFLAGS)  $(CPPFLAGS) -O -o wordplay wordplay.c  -licuio -licui18n -licuuc -licudata

