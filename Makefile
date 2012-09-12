all: trampoline test-bundle

test.squashfs:
	rm -f test.squashfs
	mksquashfs test test.squashfs

trampoline:
	gcc trampoline.c -O2 -Wall -o trampoline

test-bundle: trampoline test.squashfs
	cp trampoline test-bundle
	objcopy --add-section .bundle=test.squashfs test-bundle

clean:
	rm -f test.squashfs trampoline test-bundle
