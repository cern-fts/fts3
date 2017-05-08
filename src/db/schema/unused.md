# Unused database columns/tables

Update this document when they become used, or are dropped/renamed.

## t_se
The name is a foreign key of t_group_member, but not a single field
of the t_se table seems to be used at all.

## t_vo_acl
Seems used, but would have to be modified directly on the DB.
Seems to be some sort of way of granting access to a whole VO to a
specific user.
