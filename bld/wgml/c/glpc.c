/****************************************************************************
*
*                            Open Watcom Project
*
*  Copyright (c) 2004-2009 The Open Watcom Contributors. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description: WGML implement :P and :PC tags for LAYOUT processing
*
****************************************************************************/


#include "wgml.h"

#include "clibext.h"


/***************************************************************************/
/*   :P and :PC    attributes                                              */
/***************************************************************************/
const   lay_att     p_att[4] =
    { e_line_indent, e_pre_skip, e_post_skip, e_dummy_zero };


/********************************************************************************/
/*Define the characteristics of the paragraph entity.                           */
/*                                                                              */
/*:P                                                                            */
/*        line_indent = 0                                                       */
/*        pre_skip = 1                                                          */
/*        post_skip = 0                                                         */
/*                                                                              */
/*line_indent The line_indent attribute accepts any valid horizontal space unit.*/
/*This attribute specifies the amount of indentation for the first output       */
/*line of the paragraph.                                                        */
/*                                                                              */
/*pre_skip This attribute accepts vertical space units. A zero value means that */
/*no lines are skipped. If the skip value is a line unit, it is multiplied      */
/*by the current line spacing (see "Vertical Space Unit" on page 77 for         */
/*more information). The resulting amount of space is skipped before            */
/*the paragraph. The pre-skip will be merged with the previous                  */
/*document entity's post-skip value. If a pre-skip occurs at the                */
/*beginning of an output page, the pre-skip value has no effect.                */
/*                                                                              */
/*post_skip This attribute accepts vertical space units. A zero value means that*/
/*no lines are skipped. If the skip value is a line unit, it is multiplied      */
/*by the current line spacing (see "Vertical Space Unit" on page 77 for         */
/*more information). The resulting amount of space is skipped after             */
/*the paragraph. The post-skip will be merged with the next                     */
/*document entity's pre-skip value. If a post-skip occurs at the end of         */
/*an output page, any remaining part of the skip is not carried over to         */
/*the next output page.                                                         */
/********************************************************************************/


/********************************************************************************/
/*Define the characteristics of the paragraph continuation entity.              */
/*:PC                                                                           */
/*        line_indent = 0                                                       */
/*        pre_skip = 1                                                          */
/*        post_skip = 0                                                         */
/*                                                                              */
/*line_indent The line_indent attribute accepts any valid horizontal space unit.*/
/*This attribute specifies the amount of indentation for the first output       */
/*line of the paragraph continuation.                                           */
/*                                                                              */
/*pre_skip This attribute accepts vertical space units. A zero value means that */
/*no lines are skipped. If the skip value is a line unit, it is multiplied      */
/*by the current line spacing (see "Vertical Space Unit" on page 77 for         */
/*more information). The resulting amount of space is skipped before            */
/*the paragraph continuation. The pre-skip will be merged with the              */
/*previous document entity's post-skip value. If a pre-skip occurs at           */
/*the beginning of an output page, the pre-skip value has no effect.            */
/*                                                                              */
/*post_skip This attribute accepts vertical space units. A zero value means that*/
/*no lines are skipped. If the skip value is a line unit, it is multiplied      */
/*by the current line spacing (see "Vertical Space Unit" on page 77 for         */
/*more information). The resulting amount of space is skipped after             */
/*the paragraph continuation. The post-skip will be merged with the             */
/*next document entity's pre-skip value. If a post-skip occurs at the           */
/*end of an output page, any remaining part of the skip is not carried          */
/*over to the next output page.                                                 */
/********************************************************************************/

static  int     process_arg( p_lay_tag *p_or_pc, att_name_type *attr_name, att_val_type *attr_val )
{
    char        *   p;
    int             cvterr = -1;
    int             k;
    lay_att         curr;

    for( k = 0, curr = p_att[k]; curr > 0; k++, curr = p_att[k] ) {
        if( strcmp( lay_att_names[curr], attr_name->attname.l ) == 0 ) {
            p = attr_val->name;
            switch( curr ) {
            case e_line_indent:
                if( AttrFlags.line_indent ) {
                    xx_line_err_ci( err_att_dup, attr_name->att_name,
                        attr_val->name - attr_name->att_name + attr_val->len);
                }
                cvterr = i_space_unit( p, attr_val, &p_or_pc->line_indent );
                AttrFlags.line_indent = true;
                break;
            case e_pre_skip:
                if( AttrFlags.pre_skip ) {
                    xx_line_err_ci( err_att_dup, attr_name->att_name,
                        attr_val->name - attr_name->att_name + attr_val->len);
                }
                cvterr = i_space_unit( p, attr_val, &p_or_pc->pre_skip );
                AttrFlags.pre_skip = true;
                break;
            case e_post_skip:
                if( AttrFlags.post_skip ) {
                    xx_line_err_ci( err_att_dup, attr_name->att_name,
                        attr_val->name - attr_name->att_name + attr_val->len);
                }
                cvterr = i_space_unit( p, attr_val, &p_or_pc->post_skip );
                AttrFlags.post_skip = true;
                break;
            default:
                internal_err( __FILE__, __LINE__ );
            }
            if( cvterr ) {              // there was an error
                xx_err( err_att_val_inv );
            }
            break;                      // break out of for loop
        }
    }
    if( cvterr < 0 ) {
        xx_err( err_att_name_inv );
    }
    return( cvterr );
}


/***************************************************************************/
/*  lay_p                                                                  */
/***************************************************************************/

void    lay_p( const gmltag * entry )
{
    condcode            cc;
    int                 cvterr;
    att_name_type       attr_name;
    att_val_type        attr_val;

    (void)entry;

    if( !GlobalFlags.firstpass ) {
        scandata.s = scandata.e;
        eat_lay_sub_tag();
        return;                         // process during first pass only
    }
    memset( &AttrFlags, 0, sizeof( AttrFlags ) );   // clear all attribute flags
    if( ProcFlags.lay_xxx != el_p ) {
        ProcFlags.lay_xxx = el_p;
    }
    while( (cc = lay_attr_and_value( &attr_name, &attr_val )) == pos ) {   // get att with value
        cvterr = process_arg( &layout_work.p, &attr_name, &attr_val );
        if( ProcFlags.tag_end_found ) {
            break;
        }
    }
    scandata.s = scandata.e;
    return;
}


/***************************************************************************/
/*  lay_pc                                                                 */
/***************************************************************************/

void    lay_pc( const gmltag * entry )
{
    bool                cvterr;
    condcode            cc;
    att_name_type       attr_name;
    att_val_type        attr_val;

    (void)entry;

    if( !GlobalFlags.firstpass ) {
        scandata.s = scandata.e;
        eat_lay_sub_tag();
        return;                         // process during first pass only
    }
    memset( &AttrFlags, 0, sizeof( AttrFlags ) );   // clear all attribute flags
    if( ProcFlags.lay_xxx != el_pc ) {
        ProcFlags.lay_xxx = el_pc;
    }
    while( (cc = lay_attr_and_value( &attr_name, &attr_val )) == pos ) {   // get att with value
        cvterr = process_arg( &layout_work.pc, &attr_name, &attr_val );
        if( ProcFlags.tag_end_found ) {
            break;
        }
    }
    scandata.s = scandata.e;
    return;
}
