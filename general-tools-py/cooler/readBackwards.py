"""read a file returning the lines in reverse order for each call of readline()
This actually just reads blocks (4096 bytes by default) of data from the end of
the file and returns last line in an internal buffer.  I believe all the corner
cases are handled, but never can be sure..."""

"""This code is from `https://code.activestate.com/recipes/120686-read-a-text-file-backwards/` 
and was originally written by Matt Billenstein in the form shown in `recipe-120686-1.py`.

I (Kris) have mainly added some functionality so the BackwardsReader class can be used in a context
manager. I have also fixed the ...split('\n') lines."""

"""From the FOXSI-4 GSE repo (https://github.com/foxsi/GSE-FOXSI-4/blob/main/FoGSE/readBackwards.py).
"""

import os
import struct


class BackwardsReader:

    def __init__(self, file, blksize=4096, forward=False):
        """initialize the internal structures"""
        # get the file size
        self.size = os.stat(file).st_size # entry [6] in the output

        # how big of a block to read from the file...
        self.blksize = blksize

        # open the file
        self.f = open(file, 'rb')

        # if the file is smaller than the blocksize (or blocksize<1), read it all,
        # otherwise, read a block from the end...
        if (self.blksize<1) or (self.blksize > self.size):
            self.f.seek(-self.size, 2) # read all from end of file
        else:
            self.f.seek(-self.blksize, 2) # read from end of file
        
        self.read = self.f.read()
        self.data = self.read.split(b'\n')
        
        # b'' is added by split(b'\n') when the block begins/ends with \n
        self._remove_split_empty_strings()

        # to make sure, when iterating, we are going backwards
        if not forward:
            self.data.reverse()

        # avoid having to remove/keep track of line we gave the last time
        # just generate the next when we call
        self.data_generator = self._list_to_generator(self.data)

    def _remove_split_empty_strings(self):
        # strip the last item if it's empty...  a byproduct of the last line having
        # a newline at the end of it (b'' is added by split)
        if not self.data[-1]:
            self.data = self.data[:-1]

        # if the block starts with a new line then remove too (b'' is added by split)
        if self.data and (not self.data[0]):
            self.data = self.data[1:]

    def _list_to_generator(self, l):
        """Saves time having to remove items from data when we can just iterate."""
        for list_entry in l:
            yield list_entry

    def readline(self):
        """Effectively just works backwards through the list that is self.data"""
        # no data, then return empty
        if len(self.data) == 0:
            return ''
        
        # try to return the next line after the previous call
        # if not then empty
        try:
            return next(self.data_generator) + b'\n'
        except StopIteration:
            return ''

    def readlines(self):
        """Return all lines from self.data"""
        # List of all the lines
        return self.data
    
    def read_block(self):
        """Return full block"""
        # show full block in original order
        return self.read
    
    def close(self):
        """Make sure there is an easy way to close out."""
        self.f.close()

    def __enter__(self):
        """Enter here when using context manager and return self"""
        return self
     
    def __exit__(self, exc_type, exc_value, exc_traceback):
        """Make sure to close the opened file before leaving context manager."""
        self.close()

def read_raw_cdte(file):
    from FoGSE.telemetry_tools.parsers.CdTeparser import CdTerawalldata2parser
    # blksize=41204 for first full frame, blksize=73984 to do 2 frames, 
    # so 73984 + (73984-41204) for the next? -> correct, so 41204 bytes
    # to get to the end of the first frame, 32780 bytes thereafter
    with BackwardsReader(file=file, blksize=50_000, forward=True) as f:
        iterative_unpack=struct.iter_unpack("<I",f.read_block())
        datalist=[]
        for _,data in enumerate(iterative_unpack):

            datalist.append(data[0])

        flags, event_df, all_hkdicts = CdTerawalldata2parser(datalist)
    return flags, event_df, all_hkdicts


if(__name__ == "__main__"):
    # # do a thorough test...

    filename = "test_file0.txt"
    buffsize = 4096 # in Bytes, big enough buffer for full file

    # read file normally and then reverse the saved line order
    lines_reversed = []
    with open(filename, "rb") as f:
        line = f.readline()
        # if no line to be read then line == ""
        while line:
            #print("forward ",line)
            lines_reversed.append(line)
            line = f.readline()
    lines_reversed[-1] += b"\n" # for the match, last line doesn't have \n
    lines_reversed.reverse()

    # read file backwards now
    lines_read_backwards = []
    with BackwardsReader(file=filename, blksize=buffsize) as f:
        line = f.readline()
        # if no line to be read then line == ""
        while line:
            # print("backwards ",line)
            lines_read_backwards.append(line)
            line = f.readline()

    lines_read_backwards = []
    with BackwardsReader(file=filename, blksize=buffsize) as f:
        line = f.readline()
        # if no line to be read then line == ""
        while line:
            print("backwards ",line)
            lines_read_backwards.append(line)
            line = f.readline()

    if lines_read_backwards != lines_reversed:
        print("NOT MATCHED")
    else:
        print("MATCHED")

    lines_read_backwards_with_small_buffer0 = []
    sb = 200
    with BackwardsReader(file=filename, blksize=sb) as f:
        line = f.readline()
        # if no line to be read then line == ""
        while line:
            print(f"backwards (with small buffer:{sb}) ",line)
            lines_read_backwards_with_small_buffer0.append(line)
            line = f.readline()
    print(f"Last line of file at the top, then second last, and so on until the start of the buffer (buffer:{sb}).")

    lines_read_backwards_with_small_buffer1 = []
    sb = 186
    with BackwardsReader(file=filename, blksize=sb) as f:
        line = f.readline()
        # if no line to be read then line == ""
        while line:
            print(f"backwards (with small buffer:{sb}) ",line)
            lines_read_backwards_with_small_buffer1.append(line)
            line = f.readline()
    print(f"Last line of file at the top, then second last, and so on until the start of the buffer (buffer:{sb}).")

    # len(line.decode('utf-8')) will display the byte size of a string
