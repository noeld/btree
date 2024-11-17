# btree
A B+Tree implementation

## TODO
    [ ] complete API
    [ ] documentation
    [ ] add template parameters for less and equality of `key_type`

## Examples

### `random_inserts.cpp`

Build the `examples` target (not included in `ALL`).

Run `random_inserts`, which gives an output like below:

    sizeof(internal_node_type<Order 200>) =     1632
    sizeof(leaf_node_type<Order 110>)     =     3992
    Best order for internal nodes for page size 4096: Order  508 = 4096 bytes
    Best order for leaf nodes for     page size 4096: Order  112 = 4064 bytes
    output of btree == map => equal ☑️
    output of botree == map => equal ☑️
                Duration  :   std::map   |    btree     |  btree/map   |    botree    |  botree/map 
               insertion  :       0.697s |       0.569s |        81.6% |       0.546s |        78.3%
                 reading  :       0.810s |       0.727s |        89.7% |       0.711s |        87.8%
    Test with 1000000 key/values (TestClass/std::string)

It inserts 1 million random pairs of keys of type `TestClass` and values
of type `std::string` into 
* a `std::multimap`
* a `bt::btree` with arbitrary orders for internal nodes and leaf nodes
* a `bt::btree` with orders best (as big as possible but not bigger) for pages of 4096 bytes size.

Then, it reads all keys and values from begin to end i.e. in sorted order
from the three containers and writes to a string output. The processes are 
timed and compared.
