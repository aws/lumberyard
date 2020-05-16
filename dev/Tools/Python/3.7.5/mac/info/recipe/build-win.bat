if "%CONDA_PREFIX%"=="" (
%UserProfile%\mc\Scripts\activate
)
:: SUFFIX=-dbg
pushd %UserProfile%\conda\aggregate\python-feedstock\recipe
rmdir /s /q %UserProfile%\conda\python-build%SUFFIX%
mkdir %UserProfile%\conda\python-build%SUFFIX%\src_cache\
copy  %UserProfile%\mc\conda-bld\src_cache\Python-3.7.2_df6ec36011.tar.xz %UserProfile%\conda\python-build%SUFFIX%\src_cache\
copy  %UserProfile%\mc\conda-bld\src_cache\tcl-core-8.6.8.0_91fd3ea97f.zip %UserProfile%\conda\python-build%SUFFIX%\src_cache\
copy  %UserProfile%\mc\conda-bld\src_cache\tcltk-8.6.8.0_63a28d9112.zip %UserProfile%\conda\python-build%SUFFIX%\src_cache\
copy  %UserProfile%\mc\conda-bld\src_cache\tix-8.4.3.6_e558e3dc5e.zip %UserProfile%\conda\python-build%SUFFIX%\src_cache\
copy  %UserProfile%\mc\conda-bld\src_cache\tk-8.6.8.0_584fbfdc3c.zip %UserProfile%\conda\python-build%SUFFIX%\src_cache\
copy  %UserProfile%\mc\conda-bld\src_cache\zlib-1.2.11_debb195294.zip %UserProfile%\conda\python-build%SUFFIX%\src_cache\
copy  %UserProfile%\mc\conda-bld\src_cache\xz-5.2.2_02b6d6f1e0.zip %UserProfile%\conda\python-build%SUFFIX%\src_cache\
copy  %UserProfile%\mc\conda-bld\src_cache\bzip2-1.0.6_c42fd1432a.zip %UserProfile%\conda\python-build%SUFFIX%\src_cache\
copy  %UserProfile%\mc\conda-bld\src_cache\nasm-2.11.06_de3c87b26a.zip %UserProfile%\conda\python-build%SUFFIX%\src_cache\
conda-build . -m %UserProfile%\conda\aggregate\conda_build_config%SUFFIX%.yaml --croot %UserProfile%\conda\python-build%SUFFIX% --no-build-id 2>&1 | C:\msys64\usr\bin\tee.exe %UserProfile%\conda\python-3.7.2%SUFFIX%.log
