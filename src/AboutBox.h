/*
 * Copyright 2026, Kevin Adams. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NFS_MOUNT_ABOUT_BOX_H
#define NFS_MOUNT_ABOUT_BOX_H


// Build and Show() the standard Haiku BAboutWindow for NFSMount.
// The window owns itself and quits cleanly when closed, so the
// caller doesn't need to retain a pointer.
void show_about_window();


#endif
