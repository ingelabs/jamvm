dnl Determine whether the scandir filter takes a const struct dirent pointer.
AC_DEFUN([JAMVM_SCANDIR_CONST], [
    AC_CACHE_CHECK([for scandir filter argument type],
        [jamvm_cv_scandir_filter], [
        jamvm_save_werror=$ac_c_werror_flag
        AC_LANG_WERROR
        jamvm_cv_scandir_filter=unsupported

        for jamvm_scandir_const in const ''; do
            AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
                #include <sys/types.h>
                #include <dirent.h>

                static int filter($jamvm_scandir_const struct dirent *entry) {
                    return entry != 0;
                }
            ]], [[
                struct dirent **entries;
                return scandir(".", &entries, filter, 0);
            ]])], [
                AS_IF([test -n "$jamvm_scandir_const"],
                    [jamvm_cv_scandir_filter=const],
                    [jamvm_cv_scandir_filter=mutable])
                break
            ])
        done

        ac_c_werror_flag=$jamvm_save_werror
    ])

    AS_CASE([$jamvm_cv_scandir_filter],
        [const], [jamvm_scandir_const=const],
        [mutable], [jamvm_scandir_const=],
        [AC_MSG_ERROR([cannot determine scandir filter argument type])])

    AC_DEFINE_UNQUOTED([SCANDIR_FILTER_CONST], [$jamvm_scandir_const],
        [Define to const or empty to match the scandir filter argument.])
])
