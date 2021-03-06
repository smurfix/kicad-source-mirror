/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "picker_tool.h"
#include "pcb_actions.h"
#include "grid_helper.h"
#include <pcbnew_id.h>
#include <pcb_edit_frame.h>
#include <view/view_controls.h>
#include <tool/tool_manager.h>
#include "tool_event_utils.h"
#include "selection_tool.h"


extern bool Magnetize( PCB_BASE_EDIT_FRAME* frame, int aCurrentTool,
                       wxSize aGridSize, wxPoint on_grid, wxPoint* curpos );


TOOL_ACTION PCB_ACTIONS::pickerTool( "pcbnew.Picker", AS_GLOBAL, 0, "", "", NULL, AF_ACTIVATE );


PICKER_TOOL::PICKER_TOOL()
    : PCB_TOOL( "pcbnew.Picker" )
{
    reset();
}


int PICKER_TOOL::Main( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    GRID_HELPER grid( frame() );

    setControls();

    while( OPT_TOOL_EVENT evt = Wait() )
    {
        // TODO: magnetic pad & track processing needs to move to VIEW_CONTROLS.
        wxPoint pos( controls->GetMousePosition().x, controls->GetMousePosition().y );
        frame()->SetMousePosition( pos );

        wxRealPoint gridSize = frame()->GetScreen()->GetGridSize();
        wxSize igridsize;
        igridsize.x = KiROUND( gridSize.x );
        igridsize.y = KiROUND( gridSize.y );

        if( Magnetize( frame(), ID_PCB_HIGHLIGHT_BUTT, igridsize, pos, &pos ) )
            controls->ForceCursorPosition( true, pos );
        else
            controls->ForceCursorPosition( false );

        if( evt->IsClick( BUT_LEFT ) )
        {
            bool getNext = false;

            m_picked = VECTOR2D( controls->GetCursorPosition() );

            if( m_clickHandler )
            {
                try
                {
                    getNext = (*m_clickHandler)( *m_picked );
                }
                catch( std::exception& e )
                {
                    std::cerr << "PICKER_TOOL click handler error: " << e.what() << std::endl;
                    break;
                }
            }

            if( !getNext )
                break;
            else
                setControls();
        }

        else if( evt->IsCancel() || TOOL_EVT_UTILS::IsCancelInteractive( *evt ) || evt->IsActivate() )
        {
            if( m_cancelHandler )
            {
                try
                {
                    (*m_cancelHandler)();
                }
                catch( std::exception& e )
                {
                    std::cerr << "PICKER_TOOL cancel handler error: " << e.what() << std::endl;
                }
            }

            break;
        }

        else if( evt->IsClick( BUT_RIGHT ) )
            m_menu.ShowContextMenu();

        else
            m_toolMgr->PassEvent();
    }

    reset();
    controls->ForceCursorPosition( false );
    getEditFrame<PCB_BASE_FRAME>()->SetNoToolSelected();

    return 0;
}


void PICKER_TOOL::setTransitions()
{
    Go( &PICKER_TOOL::Main, PCB_ACTIONS::pickerTool.MakeEvent() );
}


void PICKER_TOOL::reset()
{
    m_cursorSnapping = true;
    m_cursorVisible = true;
    m_cursorCapture = false;
    m_autoPanning = false;

    m_picked = NULLOPT;
    m_clickHandler = NULLOPT;
    m_cancelHandler = NULLOPT;
}


void PICKER_TOOL::setControls()
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();

    controls->ShowCursor( m_cursorVisible );
    controls->SetSnapping( m_cursorSnapping );
    controls->CaptureCursor( m_cursorCapture );
    controls->SetAutoPan( m_autoPanning );
}
