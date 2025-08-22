src = $(wildcard *.cpp)
obj = $(src:.cpp=.o)

merge_files: $(obj)
	g++ -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) merge_files
