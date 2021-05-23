# Smart Espresso Machine ☕

IOT sim

## Prerequisites

#### Ubuntu 
- Can be accessed through a WSL or a virtual machine with Linux. ***This is needed for Pistache, as it has no Windows support.***

#### Updated apt
- How to update the package list:

```bash
sudo apt update
```

#### C++ Compiler
- The build was tested using gcc version **9.3.0**. The following command installs the necessary packages including **gcc, g++, make:**

```bash
sudo apt install build-essential
```

#### Pistache Library
- How To Install Pistache:

```bash
sudo add-apt-repository ppa:pistache+team/unstable
sudo apt install libpistache-dev
```

#### Make
- How To Install Make:

```bash
sudo apt install make
```

#### Postman
- Postman is used for testing.

## Build & Run

#### Build option 1
- Build the espressor executable by running **make** from the build folder.

#### Build option 2
- Open up a terminal window, and navigate to the build folder. Enter the following command in the terminal to build the object:
```bash
g++ espressor.cpp -o espressor -lpistache -lcrypto -lssl -lpthread
```
- The command will compile the source code using g++, and a binary executable file named "espressor" will be created.

- ***Should you encounter the following error, add "-std=g++17" to the execution parameters.***
  - ⚠️ error: 'optional' in namespace 'std' does not name a template type 



## Running the program

After successfully building the program, enter the following line in the terminal:


```bash
./espressor
```
On a successful run, there will be an output in the terminal containing the numbers of cores that are used.

## Testing
Testing is done using the Postman tool.

- For a POST request to http://localhost:9080/settings/sugar/2, the response should be "sugar was set to 2".

- For a GET request to http://localhost:9080/settings/sugar, the response should be "sugar is 2".
