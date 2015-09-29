if (!defined &_STDIO_H) {
    eval 'sub _STDIO_H {1;}';
    require 'cfuncproto.ph';
    if (defined &KERNEL) {
	require 'sprite.ph';
    }
    if (!defined &EOF) {
	eval 'sub EOF {(-1);}';
    }
    if (!defined &NULL) {
	eval 'sub NULL {0;}';
    }
    if (!defined &_CLIENTDATA) {
	eval 'sub _CLIENTDATA {1;}';
    }
    if (!defined &_VA_LIST) {
	eval 'sub _VA_LIST {1;}';
    }
    eval 'sub STDIO_READ {1;}';
    eval 'sub STDIO_WRITE {2;}';
    eval 'sub STDIO_EOF {4;}';
    eval 'sub STDIO_LINEBUF {8;}';
    eval 'sub STDIO_NOT_OUR_BUF {16;}';
    if (!defined &lint) {
	eval 'sub getc {
	    local($stream) = @_;
	    eval "((($stream)-> &readCount <= 0) ?  &fgetc($stream) : (($stream)-> &readCount -= 1, ($stream)-> &lastAccess += 1, *(($stream)-> &lastAccess)))";
	}';
	eval 'sub putc {
	    local($c, $stream) = @_;
	    eval "(((($stream)-> &writeCount <= 1) || (($stream)-> &flags &  &STDIO_LINEBUF)) ?  &fputc($c, $stream) : (($stream)-> &writeCount -= 1, ($stream)-> &lastAccess += 1, *($stream)-> &lastAccess = $c))";
	}';
    }
    else {
    }
    eval 'sub getchar {
        eval " &getc( &stdin)";
    }';
    eval 'sub putchar {
        local($c) = @_;
        eval " &putc($c,  &stdout)";
    }';
    eval 'sub ferror {
        local($stream) = @_;
        eval "(($stream)-> &status)";
    }';
    eval 'sub feof {
        local($stream) = @_;
        eval "(($stream)-> &flags &  &STDIO_EOF)";
    }';
    eval 'sub stdin {(& &stdioInFile);}';
    eval 'sub stdout {(& &stdioOutFile);}';
    eval 'sub stderr {(& &stdioErrFile);}';
    eval 'sub BUFSIZ {4096;}';
    eval 'sub _IOFBF {1;}';
    eval 'sub _IOLBF {2;}';
    eval 'sub _IONBF {3;}';
    eval 'sub SEEK_SET {0;}';
    eval 'sub SEEK_CUR {1;}';
    eval 'sub SEEK_END {2;}';
    if (defined &KERNEL) {
    }
    else {
    }
}
1;
