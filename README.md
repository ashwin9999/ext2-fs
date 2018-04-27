# OS_project
Implementation of a program that copies files into and out of a VirtualBox VDI file containing a Linux ext2 filesystem.

- To build and run:
     * Run ```make``` to compile the code using c++ 11.
     * Run ```./vdi [filename]``` to run the program, file can be a static or a dynamic vdi file.
     * Run ```make clean``` to clean up the executables.

- A few notes about the project:
    * We made separate files for all the structs because it was easier for us to debug and organize our code that way in stead of using an ext2fs.h with all the structs in it.
    * We used lseek, read, write for everything separately rather than using the vdiSeek and vdiRead (which we only used for the header). Solely for easier debugging.
    * We referred to a lot of websites for aid:
      - Source: http://cs.smith.edu/~nhowe/262/oldlabs/ext2.html#locate_file
      - Source: https://courses.cs.washington.edu/courses/cse451/09sp/projects/project3light/project3_light.html
      - Source: https://www.tldp.org/LDP/tlk/fs/filesystem.html
      and of course, stack overflow and development forums.
    * Note on running read and write operations:
      - When we run the read, we have to be in the root directory and specify the full path.
      - Same for write.
      - So, something to try would be this:

      ```
      Current path: /
      read /examples/06.Sensors/Ping/Ping.txt /Users/ashwinmishra/Desktop/final.txt
      The file /examples/06.Sensors/Ping/Ping.txt has been copied to /Users/ashwinmishra/Desktop/final.txt
      ```

      ```
      Current path: /
      write /Users/ashwinmishra/Desktop/final.txt /final.txt         
      File /Users/ashwinmishra/Desktop/final.txt has been copied to /final.txt
      ```

      - Also, when you write, the content might not be listed immediately. You have to quit. And re-run the program, and then ls to see the added content.
      - Or, you can use this to verfiy that write worked: http://www.vmxray.com/

      
