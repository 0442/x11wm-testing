//#include <X11/X.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <math.h>

static xcb_connection_t* conn;
static xcb_screen_t* screen;

xcb_connection_t* connect_to_x() {
    conn = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(conn) != 0) {
        printf("An error occured establishing a connection to X server. Exiting...\n");
        exit(1);
    }
    printf("Connected successfully to X server.\n");
    fflush(stdout);
    return conn;
}

xcb_screen_t* setup_x() {
    const xcb_setup_t* setup = xcb_get_setup(conn);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    screen = iter.data;
    uint16_t mask = XCB_CW_EVENT_MASK;
    uint32_t valwin[1];
    valwin[0] = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT;
    xcb_change_window_attributes(conn, screen->root, mask, valwin);
    /*hints*/
    uint8_t atm;
    xcb_intern_atom(conn, atm, sizeof("hello"), "hello");
    return screen;
}

/*handling notifications*/

void create_notify(xcb_generic_event_t* event) {
    xcb_create_notify_event_t* crn = (xcb_create_notify_event_t*)event; 
    printf("\nXCB_CREATE_NOTIFY\nx = %d\ny = %d\nwidth = %d\nheight = %d\nwindow: %d\n", crn->x, crn->y, crn->width, crn->height, crn->window);

    printf("override redirect: %d\n", crn->override_redirect);
    if (crn->override_redirect == False) {
        uint16_t mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
        uint32_t valwin[] = {
            10,
            10,
            100,
            100
        };
        xcb_configure_window(conn, crn->window, mask, valwin);

    } else/*center the window*/ {
        uint16_t mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
        uint32_t valwin[] = {
            (screen->width_in_pixels - crn->width) / 2,
            (screen->height_in_pixels - crn->height) / 2,
            crn->width,
            crn->height
        };
        xcb_configure_window(conn, crn->window, mask, valwin);
    }

    return;
}

void configure_notify() {
    printf("\nXCB_CONFIGURE_NOTIFY\n");
    return;
}

void visibility_notify() {
    printf("XCB_VISIBILITY_NOTIFY");
    return;
}

void map_notify(xcb_generic_event_t* event) {
    xcb_map_notify_event_t* mapn = (xcb_map_notify_event_t*)event;
    printf("\nXCB_MAP_NOTIFY\n");
    return;
}

void unmap_notify(xcb_generic_event_t* event) {
    xcb_unmap_notify_event_t* umap = (xcb_unmap_notify_event_t*)event;
    printf("\nXCB_UNMAP_NOTIFY\n");
    return;
}

void destroy_notify(xcb_generic_event_t* event) {
    xcb_destroy_notify_event_t* dstr = (xcb_destroy_notify_event_t*)event;
    printf("\nXCB_DESTROY_NOTIFY\n");
    return;
}

/*handling requests*/

void configure_request(xcb_generic_event_t* event) {
    printf("\nXCB_CONFIGURE_REQUEST\n");
    xcb_configure_request_event_t* confr = (xcb_configure_request_event_t*)event;

    uint16_t mask;
    uint32_t valwin[14];
    int i = 0;

    if ( (confr->value_mask & XCB_CONFIG_WINDOW_X) == XCB_CONFIG_WINDOW_X) {
        mask |= XCB_CONFIG_WINDOW_X;
        valwin[i] = confr->x;
        i++;
    }
    if ( (confr->value_mask & XCB_CONFIG_WINDOW_Y) == XCB_CONFIG_WINDOW_Y) {
        mask |= XCB_CONFIG_WINDOW_Y;
        valwin[i] = confr->y;
        i++;
    }
    if ( (confr->value_mask & XCB_CONFIG_WINDOW_WIDTH) == XCB_CONFIG_WINDOW_WIDTH) {
        mask |= XCB_CONFIG_WINDOW_WIDTH;
        valwin[i] = confr->width;
        i++;
    }
    if ( (confr->value_mask & XCB_CONFIG_WINDOW_HEIGHT) == XCB_CONFIG_WINDOW_HEIGHT) {
        mask |= XCB_CONFIG_WINDOW_HEIGHT;
        valwin[i] = confr->height;
        i++;
    }
    xcb_configure_window(conn, confr->window, mask, valwin);
    return;
}

void map_request(xcb_generic_event_t* event) {
    xcb_map_request_event_t* mapr = (xcb_map_request_event_t*)event;
    xcb_map_window(conn, mapr->window);
    printf("\nXCB_MAP_REQUEST\nwindow: %d\n", mapr->window);

    /*check if override redirect flag is set and proceed accordingly.*/
    xcb_get_window_attributes_cookie_t ck = xcb_get_window_attributes(conn, mapr->window);
    xcb_get_window_attributes_reply_t* attr = xcb_get_window_attributes_reply(conn, ck, NULL);

        EWMH (extended window manager hints):

    if (attr->override_redirect == True) {
        printf("has override redirect flag(map req). Not doing mods.\n");
    } else {
        uint16_t mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
        uint32_t valwin[] = {
            50,
            50,
            500,
            500
        };

        xcb_configure_window(conn, mapr->window, mask, valwin);
        printf("doesnt have override redirect flag(map req). Modding.\n");
    }

    return;
}

void resize_request() {
    printf("\nXCB_RESIZE_REQUEST\n");
    return;
}

void event_loop() {
    xcb_generic_event_t* event;
    while ( (event = xcb_wait_for_event(conn)) ) {
        switch ( event->response_type) {
            /*notifications*/
            case XCB_CREATE_NOTIFY: {
                create_notify(event);
                break;
            }

            case XCB_CONFIGURE_NOTIFY: {
                configure_notify();
                break;
            }

            case XCB_VISIBILITY_NOTIFY: {
                visibility_notify();
                break;
            }

            case XCB_MAP_NOTIFY: {
                map_notify(event);
                break;
            }

            case XCB_UNMAP_NOTIFY: {
                unmap_notify(event);
                break;
            }

            case XCB_DESTROY_NOTIFY: {
                destroy_notify(event);
                break;
            } 

            /*requests*/
            case XCB_CONFIGURE_REQUEST: {
                configure_request(event);
                break;
            }

            case XCB_RESIZE_REQUEST: {
                resize_request();
                break;
            }

            case XCB_MAP_REQUEST: {
                map_request(event);
                break;
            }

            default: {
                if (event != NULL) {
                    int msb = 1 << (int) log2(event->response_type); /*most significant bit*/
                    int ert = event->response_type;
                    int ertmsb = ert - msb;
                    printf("\nunknown event: %d\nmsb: %d\nevent – msb: %d\n", ert , msb , ertmsb);
                }
                break;
            }
        }
        free(event);
        xcb_flush(conn);
        fflush(stdout);
    }
    return;
}

int main() {
    xcb_connection_t* conn = connect_to_x();
    setup_x();

    xcb_flush(conn);

    event_loop();

    uint8_t onlyIf;
    char nam[] = "_NET_DESKTOP_GEOMETRY";
    uint16_t nameLen = strlen(nam);
    xcb_intern_atom_cookie_t kok = xcb_intern_atom(conn, onlyIf, nameLen, nam);
    xcb_intern_atom_reply_t* rep; 
    
    
    xcb_atom_t* listPropAtom = xcb_list_properties_atoms(xcb_list_properties_reply(conn, xcb_list_properties(conn, screen->root), NULL));
    printf("list properties atom: %d\n", *listPropAtom);

    xcb_disconnect(conn);
    return 0;
}
