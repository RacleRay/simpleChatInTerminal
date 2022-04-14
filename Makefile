default:all

.DEFAULT:
	@cd ./server && $(MAKE) $@
	@cd ./client && $(MAKE) $@

.PHONY:
	default
