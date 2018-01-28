Ogre Format Analyzer
====================

This is a stand alone, no-dependencies parser and writer to the binary mesh format used by the Ogre graphics engine

It can:

* read ogre files
* detect and fix section size errors
* remove mesh fields and find redundant ones
* unify multiple buffers into a single buffer
* save the files in ogre format

The main purpose of this project is to provide a fast and easy way to modify Ogre mesh files without linking to the full ogre library. The parser and writer are light weight, unlike the ogre library equivalents.
