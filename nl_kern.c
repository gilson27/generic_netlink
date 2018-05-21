#include <net/genetlink.h>
#include <linux/module.h>
#include <linux/kernel.h>

/* attributes (variables):
 * the index in this enum is used as a reference for the type,
 * userspace application has to indicate the corresponding type
 * the policy is used for security considerations 
 */
enum {
    FAMILY1_A_UNSPEC,
    FAMILY1_A_MSG,
    __FAMILY1_A_MAX,
};
#define FAMILY1_A_MAX (__FAMILY1_A_MAX - 1)

/* attribute policy: defines which attribute has which type (e.g int, char * etc)
 * possible values defined in net/netlink.h 
 */

static struct nla_policy family1_genl_policy[FAMILY1_A_MAX + 1] = {
    [FAMILY1_A_MSG] = { .type = NLA_STRING, .len=1},
};

/* 
 *Attributes and policies for family 2
 */
enum {
    FAMILY2_A_UNSPEC,
    FAMILY2_A_MSG,
    __FAMILY2_A_MAX,
};
#define FAMILY2_A_MAX (__FAMILY2_A_MAX - 1)

static struct nla_policy family2_genl_policy[FAMILY2_A_MAX + 1] = {
    [FAMILY2_A_MSG] = { .type = NLA_NUL_STRING },
};

/* 
 * Attributes and policies for family 3
 */
enum {
    FAMILY3_A_UNSPEC,
    FAMILY3_A_MSG,
    __FAMILY3_A_MAX,
};
#define FAMILY3_A_MAX (__FAMILY3_A_MAX - 1)

static struct nla_policy family3_genl_policy[FAMILY3_A_MAX + 1] = {
    [FAMILY3_A_MSG] = { .type = NLA_NUL_STRING, .len=100 },
};

// static struct nla_policy family3_genl_policy[FAMILY3_A_MAX + 1] = {
//     [FAMILY3_A_MSG] = { .type = NLA_U32 },
// };

// static struct nla_policy family3_genl_policy[FAMILY3_A_MAX + 1] = {
//     [FAMILY3_A_MSG] = { .type = NLA_UNSPEC },
// };

/**
 * Version Number of various families
 */
#define FAMILY1_VERSION_NR 1
#define FAMILY2_VERSION_NR 1
#define FAMILY3_VERSION_NR 1

/*
 * Family definition
 */
static struct genl_family family1_gnl_family = {
    .id = GENL_ID_GENERATE,         
    .hdrsize = 0,
    .name = "FAMILY1",        
    .version = FAMILY1_VERSION_NR,  
    .maxattr = FAMILY1_A_MAX,
};

static struct genl_family family2_gnl_family = {
    .id = GENL_ID_GENERATE,
    .hdrsize = 0,
    .name = "FAMILY2",     
    .version = FAMILY2_VERSION_NR,  
    .maxattr = FAMILY2_A_MAX,
};

static struct genl_family family3_gnl_family = {
    .id = GENL_ID_GENERATE,      
    .hdrsize = 0,
    .name = "FAMILY3",
    .version = FAMILY3_VERSION_NR,
    .maxattr = FAMILY3_A_MAX,
};

/* commands: enumeration of all commands (functions), 
 * used by userspace application to identify command to be executed
 */
enum {
    FAMILY_C_UNSPEC,
    FAMILY_C_SEND,
    FAMILY_C_RECV,
    __FAMILY_C_MAX,
};
#define FAMILY_C_MAX (__FAMILY_C_MAX - 1)


/**
 * Callback for hadnling family1 data reception
*/
int family1_recv(struct sk_buff *skb_temp, struct genl_info *info) {
    struct nlattr *na, *validate_attrs[FAMILY1_A_MAX + 1];
    struct sk_buff *skb;
    int rc, is_valid;
    void *msg_head;
    char * recvd_data;
    int *recvd_int;
    
    if (info == NULL) {
        printk("Info is null. \n");
        goto out;
    }

  
    /* For each attribute there is an index in info->attrs which points to a nlattr structure
     * in this structure the data is given
     */
    na = info->attrs[FAMILY1_A_MSG];
    if (na) {
        /**
         * Validate attributes
        */
        is_valid = nla_validate(na, na->nla_len, FAMILY1_A_MAX, family1_genl_policy);
        printk("is_valid=%d na->nla_len=%d\n", is_valid, na->nla_len);


        recvd_data = (char *)nla_data(na);
        if (recvd_data == NULL) {
            printk("error while receiving data\n");
        } else {
            printk("received: %s\n", recvd_data);
        }
        // recvd_int = (int *)nla_data(na);
        // printk("received: %d\n", *recvd_int);
    } else {
        printk("no info->attrs %i\n", FAMILY1_A_MSG);
    }



    /* Send a message back
     * Allocate some memory, since the size is not yet known use NLMSG_GOODSIZE
     */
    skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
    if (skb == NULL) {
        goto out;
    }

    //Create the message headers
    /* arguments of genlmsg_put: 
       struct sk_buff *, 
       int (sending) pid, 
       int sequence number, 
       struct genl_family *, 
       int flags, 
       u8 command index (why do we need this?)
    */
    msg_head = genlmsg_put(skb, 0, info->snd_seq+1, &family1_gnl_family, 0, FAMILY_C_RECV);
    if (msg_head == NULL) {
        rc = -ENOMEM;
        goto out;
    }
    //Add a FAMILY1_A_MSG attribute (actual value to be sent)
    rc = nla_put_string(skb, FAMILY1_A_MSG, "Reply from family 1");
    if (rc != 0) {
        goto out;
    }
    
    //Finalize the message
    genlmsg_end(skb, msg_head);

    //Send the message back
    rc = genlmsg_unicast(genl_info_net(info), skb, info->snd_portid );
    if (rc != 0) {
        goto out;
    }
    return 0;

    out:
    printk("An error occured in family 1 receive:\n");
    return 0;
}

/*
 * Commands: mapping between the command enumeration and the actual function
 */
struct genl_ops family1_gnl_ops_recv = {
    .cmd = FAMILY_C_SEND,
    .flags = 0,
    .policy = family1_genl_policy,
    .doit = family1_recv,
    .dumpit = NULL,
};

/**
 * Callback for hadnling family2 data reception
*/
int family2_recv(struct sk_buff *skb_temp, struct genl_info *info) {
    struct nlattr *na;
    struct sk_buff *skb;
    int rc;
    void *msg_head;
    char * recvd_data;
    
    if (info == NULL) {
        goto out;
    }
  
    /* For each attribute there is an index in info->attrs which points to a nlattr structure
     * in this structure the data is given
     */
    na = info->attrs[FAMILY2_A_MSG];
    if (na) {
        recvd_data = (char *)nla_data(na);
        if (recvd_data == NULL) {
            printk("error while receiving data\n");
        } else {
            printk("received: %s\n", recvd_data);
        }
    } else {
        printk("no info->attrs %i\n", FAMILY2_A_MSG);
    }

    /* Send a message back
     * Allocate some memory, since the size is not yet known use NLMSG_GOODSIZE
     */
    skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
    if (skb == NULL) {
        goto out;
    }

    //Create the message headers
    /* arguments of genlmsg_put: 
       struct sk_buff *, 
       int (sending) pid, 
       int sequence number, 
       struct genl_family *, 
       int flags, 
       u8 command index (why do we need this?)
    */
    msg_head = genlmsg_put(skb, 0, info->snd_seq+1, &family2_gnl_family, 0, FAMILY_C_RECV);
    if (msg_head == NULL) {
        rc = -ENOMEM;
        goto out;
    }
    //Add a FAMILY1_A_MSG attribute (actual value to be sent)
    rc = nla_put_string(skb, FAMILY2_A_MSG, "Reply from family 2");
    if (rc != 0) {
        goto out;
    }
    
    //Finalize the message
    genlmsg_end(skb, msg_head);

    //Send the message back
    rc = genlmsg_unicast(genl_info_net(info), skb, info->snd_portid );
    if (rc != 0) {
        goto out;
    }
    return 0;

    out:
    printk("An error occured in family 2 receive:\n");
    return 0;
}

/*
 * Commands: mapping between the command enumeration and the actual function
 */
struct genl_ops family2_gnl_ops_recv = {
    .cmd = FAMILY_C_SEND,
    .flags = 0,
    .policy = family2_genl_policy,
    .doit = family2_recv,
    .dumpit = NULL,
};

/**
 * Callback for hadnling family2 data reception
*/
int family3_recv(struct sk_buff *skb_temp, struct genl_info *info) {
    struct nlattr *na;
    struct sk_buff *skb;
    int rc;
    void *msg_head;
    char * recvd_data;
    
    if (info == NULL) {
        goto out;
    }
  
    /* For each attribute there is an index in info->attrs which points to a nlattr structure
     * in this structure the data is given
     */
    na = info->attrs[FAMILY3_A_MSG];
    if (na) {
        recvd_data = (char *)nla_data(na);
        if (recvd_data == NULL) {
            printk("error while receiving data\n");
        } else {
            printk("received: %s\n", recvd_data);
        }
    } else {
        printk("no info->attrs %i\n", FAMILY2_A_MSG);
    }

    /* Send a message back
     * Allocate some memory, since the size is not yet known use NLMSG_GOODSIZE
     */
    skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
    if (skb == NULL) {
        goto out;
    }

    //Create the message headers
    /* arguments of genlmsg_put: 
       struct sk_buff *, 
       int (sending) pid, 
       int sequence number, 
       struct genl_family *, 
       int flags, 
       u8 command index (why do we need this?)
    */
    msg_head = genlmsg_put(skb, 0, info->snd_seq+1, &family3_gnl_family, 0, FAMILY_C_RECV);
    if (msg_head == NULL) {
        rc = -ENOMEM;
        goto out;
    }
    //Add a FAMILY1_A_MSG attribute (actual value to be sent)
    rc = nla_put_string(skb, FAMILY3_A_MSG, "Reply from family 3");
    if (rc != 0) {
        goto out;
    }
    
    //Finalize the message
    genlmsg_end(skb, msg_head);

    //Send the message back
    rc = genlmsg_unicast(genl_info_net(info), skb, info->snd_portid );
    if (rc != 0) {
        goto out;
    }
    return 0;

    out:
    printk("An error occured in family 3 receive:\n");
    return 0;
}

/*
 * Commands: mapping between the command enumeration and the actual function
 */
struct genl_ops family3_gnl_ops_recv = {
    .cmd = FAMILY_C_SEND,
    .flags = 0,
    .policy = family3_genl_policy,
    .doit = family3_recv,
    .dumpit = NULL,
};

/**
    Module entry
*/
static int __init gnKernel_init(void) {
    int rc;
    printk("Generic Netlink Example Module inserted.\n");
           
    /*
     * Register the 3 families
     */
    rc = genl_register_family(&family1_gnl_family);
    if (rc != 0) {
        goto failure;
    }

    rc = genl_register_family(&family2_gnl_family);
    if (rc != 0) {
        goto failure;
    }

    rc = genl_register_family(&family3_gnl_family);
    if (rc != 0) {
        goto failure;
    }
    /*
     * Register functions (commands) of the 3 families
     */
    rc = genl_register_ops(&family1_gnl_family, &family1_gnl_ops_recv);
    if (rc != 0) {
        printk("Register ops f1: %i\n",rc);
        genl_unregister_family(&family1_gnl_family);
        goto failure;
    }

    rc = genl_register_ops(&family2_gnl_family, &family2_gnl_ops_recv);
    if (rc != 0) {
        printk("Register ops f2: %i\n",rc);
        genl_unregister_family(&family2_gnl_family);
        goto failure;
    }

    rc = genl_register_ops(&family3_gnl_family, &family3_gnl_ops_recv);
    if (rc != 0) {
        printk("Register ops f3: %i\n",rc);
        genl_unregister_family(&family3_gnl_family);
        goto failure;
    }
    return 0; 
failure:
    printk("An error occured while inserting the generic netlink module\n");
    return -1;
}

static void __exit gnKernel_exit(void) {
    int ret;
    printk("Generic Netlink Example Module unloaded.\n");
    
    //Unregister the functions
    ret = genl_unregister_ops(&family1_gnl_family, &family1_gnl_ops_recv);
    if(ret != 0) {
        printk("Unregister ops f1: %i\n",ret);
    }
    ret = genl_unregister_ops(&family2_gnl_family, &family2_gnl_ops_recv);
    if(ret != 0) {
        printk("Unregister ops f2: %i\n",ret);
    }
    ret = genl_unregister_ops(&family3_gnl_family, &family3_gnl_ops_recv);
    if(ret != 0) {
        printk("Unregister ops f3: %i\n",ret);
    }

    //Unregister the family
    ret = genl_unregister_family(&family1_gnl_family);
    if(ret !=0) {
        printk("Unregister family 1: %i\n",ret);
        return;
    }
    ret = genl_unregister_family(&family2_gnl_family);
    if(ret !=0) {
        printk("Unregister family 2: %i\n",ret);
        return;
    }
    ret = genl_unregister_family(&family3_gnl_family);
    if(ret !=0) {
        printk("Unregister family 3: %i\n",ret);
        return;
    }
}

module_init(gnKernel_init);
module_exit(gnKernel_exit);
MODULE_LICENSE("GPL");