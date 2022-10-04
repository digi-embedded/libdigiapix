Digi APIX Python module
=======================

This project contains the source code of the Digi APIX Python module. This
module is a collection of classes and bindings to call and execute
`libdigiapix <https://github.com/digi-embedded/libdigiapix>`_ C library
code from Python.

The `Digi APIX` Python module is part of the `Digi Embedded Yocto
<https://www.digi.com/resources/documentation/digidocs/embedded/dey/3.2/cc6ul/index.html>`_
distribution.

This source has been contributed by Digi International.


Build from sources
------------------

To build the Digi APIX module from sources, *Python 3.8* or greater is
required with the *setuptools* module installed.

You can directly execute the Python build command in the Python bindings
directory or use the existing library Makefile *python-bindings* target.

Build using *Python*::

    cd bindings/python
    python3 setup.py bdist_wheel

Build using *Makefile*::

    make python-bindings

Both methods produce the distribution **wheel** file located in
*bindings/python/dist* folder.


Installation
------------

To install the Digi APIX module, *Python 3.8* or greater is required with the
*setuptools* and *pip* modules installed.

You can directly execute the Python install command or use the existing library
Makefile *install-python-bindings* target.

Build using *Python*::

    python3 -m pip install --prefix <DESTDIR>/usr bindings/python/dist/*.whl

*Where <DESTDIR> is an optional path to install the module files in.*

Build using *Makefile*::

    make install-python-bindings


Documentation
-------------

TODO


License
-------

Copyright 2022, Digi International Inc.

The `MPL 2.0 license <https://github.com/digi-embedded/libdigiapix/blob/master/bindings/python/LICENSE.txt>`_
covers this project.

