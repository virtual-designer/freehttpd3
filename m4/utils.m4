AC_DEFUN([NETWORK_FETCH], [
    code=0

    AS_IF([test "x$CURL" != "xno"], [
        "$CURL" -fSsL "$1" -o "$2"
        code=$?
    ], [
        AS_IF([test "x$WGET" != "xno"], [
            "$WGET" -q "$1" -O "$2"
            code=$?
        ])
    ])

    AS_IF([test "$code" != "0"], [
        AC_MSG_ERROR([Failed to fetch required file: $1])
        exit 1
    ])
])

AC_DEFUN([PRINT_SUMMARY], [
	AC_MSG_NOTICE([configuration summary:

  Version:                   $VERSION
  Target Triplet:            $target
  Build Triplet:             $build
  Build type:                $build_type
  Compiler:                  $CC
  Compiler flags:            $CFLAGS
  Linker flags:              $LDFLAGS
  Preprocessor flags:        $CPPFLAGS
  Libraries:                 $LIBS
  Installation prefix:       $prefix
	])
])

AC_DEFUN([CONCAT_SPACED_STRING], [
    AS_IF([test -n "$$1"], [$1="$$1 $2"], [$1="$2"])
])
