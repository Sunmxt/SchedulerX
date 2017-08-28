#ifndef TINY_DRIVER_KERNEL_LIST
#define TINY_DRIVER_KERNEL_LIST

typedef struct forward_list_node
{ 
    struct forward_list_node *next;
}for_list_node;

typedef struct bidirectional_list_node 
{
    struct bidirectional_list_node *next;
    struct bidirectional_list_node *prev;
}bi_list_node;

#define S_LIST_STRUCT_OFFSET_OF(struct_type, member)  ((unsigned int)(&((struct_type*)0) -> member))
#define S_LIST_TO_DATA(node_pointer, struct_type, pointer_member)\
    ((struct_type*)((unsigned char*)node_pointer - S_LIST_STRUCT_OFFSET_OF(struct_type, pointer_member)))

#endif
