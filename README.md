# SD
C++ String dictionary with front coding compression and serialization

sd is a C++ class implementing a compressed string dictionary. The class provides fast add, locate and 
extract using front coding compression. Front coding compression means that common prefixes are compressed. This makes this string dictionary suitable for compact storage and fast access to a huge amount of strings like URL:s or file paths. Using sd it may be possible to hold a complete dictionary in RAM instead of having to use slower disk-based storage.

Note: 
* Strings must be added in strict ascending lexicographic order. 
* Strings are stored as raw bytes, but '\0' cant't be used in a string because that char is used as string terminator inside the data structure.
* BUCKET_SIZE may be changed to shift the trade-off between compression level and speed of access. Big BUCKET_SIZE means higher compression but slower access. Sane values are between 16 and 2048.
 
This software is simplified version of StringDictionaryPFC from libCSD:
```
==========================================================================
  "Compressed String Dictionaries"
  Nieves R. Brisaboa, Rodrigo Cánovas, Francisco Claude, 
  Miguel A. Martínez-Prieto, and Gonzalo Navarro
  10th Symposium on Experimental Algorithms (SEA'2011), p.136-147, 2011.
==========================================================================
```
See: https://github.com/migumar2/libCSD
