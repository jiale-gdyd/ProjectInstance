#include <xlib/xlib/config.h>
#include <xlib/xlib/xatomic.h>
#include <xlib/xlib/xrefcount.h>
#include <xlib/xlib/xmessages.h>

void (x_ref_count_init)(xrefcount *rc)
{
    x_return_if_fail(rc != NULL);
    *rc = -1;
}

void (x_ref_count_inc)(xrefcount *rc)
{
    xrefcount rrc;

    x_return_if_fail(rc != NULL);
    rrc = *rc;
    x_return_if_fail(rrc < 0);

    if (rrc ==X_MININT) {
        x_critical("Reference count %p has reached saturation", rc);
        return;
    }

    rrc -= 1;
    *rc = rrc;
}

xboolean (x_ref_count_dec)(xrefcount *rc)
{
    xrefcount rrc;

    x_return_val_if_fail(rc != NULL, FALSE);
    rrc = *rc;
    x_return_val_if_fail(rrc < 0, FALSE);

    rrc += 1;
    if (rrc == 0) {
        return TRUE;
    }
    *rc = rrc;

    return FALSE;
}

xboolean (x_ref_count_compare)(xrefcount *rc, xint val)
{
    xrefcount rrc;

    x_return_val_if_fail(rc != NULL, FALSE);
    x_return_val_if_fail(val >= 0, FALSE);

    rrc = *rc;
    if (val == X_MAXINT) {
        return rrc == X_MININT;
    }

    return rrc == -val;
}

void (x_atomic_ref_count_init)(xatomicrefcount *arc)
{
    x_return_if_fail(arc != NULL);
    *arc = 1;
}

void (x_atomic_ref_count_inc)(xatomicrefcount *arc)
{
    xint old_value;

    x_return_if_fail(arc != NULL);
    old_value = x_atomic_int_add(arc, 1);
    x_return_if_fail(old_value > 0);

    if (old_value == X_MAXINT) {
        x_critical("Reference count has reached saturation");
    }
}

xboolean (x_atomic_ref_count_dec)(xatomicrefcount *arc)
{
    xint old_value;

    x_return_val_if_fail(arc != NULL, FALSE);
    old_value = x_atomic_int_add(arc, -1);
    x_return_val_if_fail(old_value > 0, FALSE);

    return old_value == 1;
}

xboolean (x_atomic_ref_count_compare)(xatomicrefcount *arc, xint val)
{
    x_return_val_if_fail(arc != NULL, FALSE);
    x_return_val_if_fail(val >= 0, FALSE);

    return x_atomic_int_get(arc) == val;
}
