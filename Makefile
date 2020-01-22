SUB_DIR = pstree perf crepl ping

.PHONY: build clean

build:
	@for dir in $(SUB_DIR); do \
		$(MAKE) -C $$dir; \
	done

clean:
	@for dir in $(SUB_DIR); do \
		$(MAKE) -C $$dir clean; \
	done