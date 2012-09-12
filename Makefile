all: trampoline test-bundle helper

test.squashfs:
	rm -f test.squashfs
	mksquashfs test test.squashfs

trampoline:
	gcc trampoline.c -O2 -Wall -o trampoline

helper:
	gcc helper.c -O2 -Wall -o helper

helper-setuid:
	chown root helper
	chmod u+s helper

test-bundle: trampoline test.squashfs helper
	cp trampoline test-bundle
	objcopy --add-section .bundle=test.squashfs test-bundle

clean:
	rm -f test.squashfs trampoline test-bundle helper
