//----------------------------------------------------------------------------
//
//  This file is part of seq42.
//
//  seq42 is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  seq42 is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with seq42; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//-----------------------------------------------------------------------------

#include "trackmenu.h"
#include "seqedit.h"
#include "font.h"


// Constructor

trackmenu::trackmenu( perform *a_p  )
{    
    using namespace Menu_Helpers;

    m_mainperf = a_p;
    m_menu = NULL;

    // init the clipboard, so that we don't get a crash
    // on paste with no previous copy...
    m_clipboard.set_master_midi_bus( a_p->get_master_midi_bus() );
    m_something_to_paste = false;
}


    void
trackmenu::popup_menu( void )
{

    using namespace Menu_Helpers;

    if ( m_menu != NULL )
        delete m_menu;

    m_menu = manage( new Menu());

    if ( ! m_mainperf->is_active_track( m_current_trk )) {
        m_menu->items().push_back(MenuElem("New",
                    mem_fun(*this, &trackmenu::trk_new)));
    }

    if ( m_mainperf->is_active_track( m_current_trk )) {
        m_menu->items().push_back(MenuElem("Edit", mem_fun(*this,&trackmenu::trk_edit)));
        m_menu->items().push_back(MenuElem("New sequence", mem_fun(*this,&trackmenu::new_sequence)));
        if(! m_mainperf->is_track_in_edit( m_current_trk ))
        {
            m_menu->items().push_back(MenuElem("Cut", mem_fun(*this,&trackmenu::trk_cut)));
        }
        m_menu->items().push_back(MenuElem("Copy", mem_fun(*this,&trackmenu::trk_copy)));
    } else {
        if(m_something_to_paste) m_menu->items().push_back(MenuElem("Paste", mem_fun(*this,&trackmenu::trk_paste)));
    }

    if ( m_mainperf->is_active_track( m_current_trk )) {
        m_menu->items().push_back(SeparatorElem());
        Menu *menu_buses = manage( new Menu() );

        m_menu->items().push_back( MenuElem( "Midi Bus", *menu_buses) );

        /* midi buses */
        mastermidibus *masterbus = m_mainperf->get_master_midi_bus();
        for ( int i=0; i< masterbus->get_num_out_buses(); i++ ){

            Menu *menu_channels = manage( new Menu() );

            menu_buses->items().push_back(MenuElem( masterbus->get_midi_out_bus_name(i),
                        *menu_channels ));
            char b[4];

            /* midi channel menu */
            for( int j=0; j<16; j++ ){
                snprintf(b, sizeof(b), "%d", j + 1);
                std::string name = string(b);
                int instrument = global_user_midi_bus_definitions[i].instrument[j]; 
                if ( instrument >= 0 && instrument < c_maxBuses )
                {
                    name = name + (string(" (") + 
                            global_user_instrument_definitions[instrument].instrument + 
                            string(")") );
                }

                menu_channels->items().push_back(MenuElem(name, 
                            sigc::bind(mem_fun(*this,&trackmenu::set_bus_and_midi_channel), 
                                i, j )));
            }
        }        
    }

    m_menu->popup(0,0);

}

    void
trackmenu::set_bus_and_midi_channel( int a_bus, int a_ch )
{
    if ( m_mainperf->is_active_track( m_current_trk )) {
        m_mainperf->get_track( m_current_trk )->set_midi_bus( a_bus );
        m_mainperf->get_track( m_current_trk )->set_midi_channel( a_ch );
        m_mainperf->get_track( m_current_trk )->set_dirty();
    }
}

// Makes a New track 
void 
trackmenu::trk_new(){

    if ( ! m_mainperf->is_active_track( m_current_trk )){

        m_mainperf->new_track( m_current_trk );
        m_mainperf->get_track( m_current_trk )->set_dirty();
        // FIXME: add a bool preference: "New track pops up edit window?"
        trk_edit();

    }
}

// Copies selected to clipboard track */
void 
trackmenu::trk_copy(){

    if ( m_mainperf->is_active_track( m_current_trk )) {
        m_clipboard = *(m_mainperf->get_track( m_current_trk ));
        m_something_to_paste = true;
    }
}

// Deletes and Copies to Clipboard */
void 
trackmenu::trk_cut(){

    if ( m_mainperf->is_active_track( m_current_trk ) &&
            !m_mainperf->is_track_in_edit( m_current_trk ) ){

        m_clipboard = *(m_mainperf->get_track( m_current_trk ));
        m_something_to_paste = true;
        m_mainperf->delete_track( m_current_trk );
        redraw( m_current_trk );
    }
}

// Puts clipboard into location
void 
trackmenu::trk_paste(){

    if ( m_something_to_paste && ! m_mainperf->is_active_track( m_current_trk ))
    {
        m_mainperf->new_track( m_current_trk  );
        *(m_mainperf->get_track( m_current_trk )) = m_clipboard;
        m_mainperf->get_track( m_current_trk )->set_dirty();
    }
}

void 
trackmenu::trk_edit(){

    if ( m_mainperf->is_active_track( m_current_trk )) {
        track *a_track = m_mainperf->get_track( m_current_trk );
        if(a_track->get_editing()) {
            a_track->set_raise(true);
        } else {
            new trackedit(a_track);
        }
    }
}

void 
trackmenu::new_sequence(){
    track *a_track = m_mainperf->get_track( m_current_trk );
    int seq_idx = a_track->new_sequence();
    sequence *a_sequence = a_track->get_sequence(seq_idx);
    new seqedit( a_sequence, m_mainperf );
}


void 
trackmenu::trk_clear_perf(){

    if ( m_mainperf->is_active_track( m_current_trk )){

        m_mainperf->push_trigger_undo();
        
        m_mainperf->clear_track_triggers( m_current_trk  );
        m_mainperf->get_track( m_current_trk )->set_dirty();

    }
}



