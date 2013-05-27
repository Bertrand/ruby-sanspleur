/* Manual exports for Ruby 1.9 */

struct rb_iseq_struct;

struct rb_iseq_struct {

    enum iseq_type {
    ISEQ_TYPE_TOP,
    ISEQ_TYPE_METHOD,
    ISEQ_TYPE_BLOCK,
    ISEQ_TYPE_CLASS,
    ISEQ_TYPE_RESCUE,
    ISEQ_TYPE_ENSURE,
    ISEQ_TYPE_EVAL,
    ISEQ_TYPE_MAIN,
    ISEQ_TYPE_DEFINED_GUARD
    } type;              

    VALUE name;
    VALUE filename;
    VALUE filepath;
    VALUE *iseq;
    VALUE *iseq_encoded;
    unsigned long iseq_size;
    VALUE mark_ary; 
    VALUE coverage;     
    unsigned short line_no;

    void *insn_info_table;
    size_t insn_info_size;

    ID *local_table;
    int local_table_size;

    int local_size;

    void *ic_entries;
    int ic_size;

    int argc;
    int arg_simple;
    int arg_rest;
    int arg_block;
    int arg_opts;
    int arg_post_len;
    int arg_post_start;
    int arg_size;
    VALUE *arg_opt_table;

    size_t stack_max; 

    void *catch_table;
    int catch_table_size;

    struct rb_iseq_struct *parent_iseq;
    struct rb_iseq_struct *local_iseq;

    VALUE self;
    VALUE orig;        

    void *cref_stack;
    VALUE klass;

    ID defined_method_id; // this is the last one we need. Don't go beyond this, we only access frames as pointers.
};

typedef struct rb_iseq_struct rb_iseq_t;
typedef enum {
    NOEX_PUBLIC    = 0x00,
    NOEX_NOSUPER   = 0x01,
    NOEX_PRIVATE   = 0x02,
    NOEX_PROTECTED = 0x04,
    NOEX_MASK      = 0x06,
    NOEX_BASIC     = 0x08,
    NOEX_UNDEF     = NOEX_NOSUPER,
    NOEX_MODFUNC   = 0x12,
    NOEX_SUPER     = 0x20,
    NOEX_VCALL     = 0x40,
    NOEX_RESPONDS  = 0x80
} rb_method_flag_t;

typedef enum {
    VM_METHOD_TYPE_ISEQ,
    VM_METHOD_TYPE_CFUNC,
    VM_METHOD_TYPE_ATTRSET,
    VM_METHOD_TYPE_IVAR,
    VM_METHOD_TYPE_BMETHOD,
    VM_METHOD_TYPE_ZSUPER,
    VM_METHOD_TYPE_UNDEF,
    VM_METHOD_TYPE_NOTIMPLEMENTED,
    VM_METHOD_TYPE_OPTIMIZED, /* Kernel#send, Proc#call, etc */
    VM_METHOD_TYPE_MISSING   /* wrapper for method_missing(id) */
} rb_method_type_t;

typedef struct rb_method_definition_struct {
    rb_method_type_t type; /* method type */
    ID original_id;
} rb_method_definition_t;

typedef struct rb_method_entry_struct {
    rb_method_flag_t flag;
    char mark;
    rb_method_definition_t *def;
    ID called_id;
    VALUE klass;                    /* should be mark */
} rb_method_entry_t;

typedef struct {
    VALUE *pc;          /* cfp[0] */
    VALUE *sp;          /* cfp[1] */
    VALUE *bp;          /* cfp[2] */
    rb_iseq_t *iseq;        /* cfp[3] */
    VALUE flag;         /* cfp[4] */
    VALUE self;         /* cfp[5] / block[0] */
    VALUE *lfp;         /* cfp[6] / block[1] */
    VALUE *dfp;         /* cfp[7] / block[2] */
    rb_iseq_t *block_iseq;  /* cfp[8] / block[3] */
    VALUE proc;         /* cfp[9] / block[4] */
    const rb_method_entry_t *me;/* cfp[10] */
} rb_control_frame_t;


typedef struct rb_thread_struct {
    VALUE self;
    void /*rb_vm_t*/ *vm;

    VALUE *stack;       
    unsigned long stack_size;
    rb_control_frame_t *cfp;
} rb_thread_t;


int rb_vm_get_sourceline(const rb_control_frame_t *cfp);



#define RUBY_VM_IFUNC_P(ptr)        (BUILTIN_TYPE(ptr) == T_NODE)
#define VM_FRAME_MAGIC_MASK_BITS   8
#define VM_FRAME_MAGIC_MASK   (~(~0<<VM_FRAME_MAGIC_MASK_BITS))
#define VM_FRAME_TYPE(cfp) ((cfp)->flag & VM_FRAME_MAGIC_MASK)

#define ISEQ_FILENAME(iseq_ptr) (RSTRING_PTR(iseq_ptr->filename))
#define CFP_LINENO(cfp_ptr) (rb_vm_get_sourceline(cfp_ptr))
