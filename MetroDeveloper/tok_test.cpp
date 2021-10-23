#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

extern "C" 
{
	#include "mfile.h"
	#include "tok_file.h"
};

static int debug_info = 1;

void open_element(MFILE *f, const char *element)
{
	if(debug_info)
	{
		mfwrite_stringz("open_element", f);
		mfwrite_stringz("stringz", f);
		mfwrite_stringz(element, f);
	}
}

void close_element(MFILE *f, const char *element)
{
	if(debug_info)
	{
		mfwrite_stringz("close_element", f);
		mfwrite_stringz("stringz", f);
		mfwrite_stringz(element, f);
	}
}

void w_s32(MFILE *f, const char *name, int value)
{
	if(debug_info)
	{
		mfwrite_stringz(name, f);
		mfwrite_stringz("s32", f);
	}
	
	mfwrite(&value, 4, f);
}

void w_u32(MFILE *f, const char *name, unsigned value)
{
	if(debug_info)
	{
		mfwrite_stringz(name, f);
		mfwrite_stringz("u32", f);
	}
	
	mfwrite(&value, 4, f);
}

void w_fp32(MFILE *f, const char *name, float value)
{
	if(debug_info)
	{
		mfwrite_stringz(name, f);
		mfwrite_stringz("fp32", f);
	}
	
	mfwrite(&value, 4, f);
}

void w_bool(MFILE *f, const char *name, char value)
{
	if(debug_info)
	{
		mfwrite_stringz(name, f);
		mfwrite_stringz("bool", f);
	}
	
	mfwrite(&value, 1, f);
}

void convert_mesh3D(MFILE *out, TOKNode *node)
{
	mfwrite_char(0x10 | debug_info, out); // bin flags
	
	open_element(out, "mesh");
	
		w_s32(out, "majorRelease", getAttribute_int(node, "majorRelease", -1));
		w_s32(out, "minorRelease", getAttribute_int(node, "minorRelease", -1));
	
		// geometry
		TOKNode *mesh3D = getNode(node, "mesh3D");
		open_element(out, "mesh3D");
		
			// vertices
			TOKNode *verts = getNode(mesh3D, "verts");
			open_element(out, "verts");
			
			int z_counter = 0, vert_count = nodeCount(verts);
			float *z_array = (float*)malloc(vert_count * sizeof(float));
			
			w_s32(out, "size", vert_count);
			
			for(TOKNode *vert = verts->child; vert; vert = vert->next)
			{
				open_element(out, "vert");
				
				w_s32(out, "x", getAttribute_int(vert, "x", -1));
				w_s32(out, "y", getAttribute_int(vert, "y", -1));
				
				z_array[z_counter] = getAttribute_float(vert, "z", 0.0);
				w_fp32(out, "z", z_array[z_counter]);
					
				close_element(out, "vert");
				z_counter++;
			}
			
			close_element(out, "verts");
		
			// tris
			TOKNode *tris = getNode(mesh3D, "tris");
			open_element(out, "tris");
			
			w_s32(out, "size", nodeCount(tris));
			
			for(TOKNode *tri = tris->child; tri; tri = tri->next)
			{
				open_element(out, "tri");
				
				int v1 = getAttribute_int(tri, "edge0StartVert", -1);
				int v2 = getAttribute_int(tri, "edge1StartVert", -1);
				int v3 = getAttribute_int(tri, "edge2StartVert", -1);
				
				w_s32(out, "surfaceType", getAttribute_int(tri, "surfaceType", 1)); // default value 0, 1, or -1?
				w_s32(out, "sectionID", getAttribute_int(tri, "sectionID", -1)); // default value 0, 1, or -1?
				w_s32(out, "userData", getAttribute_int(tri, "userData", -1));
				w_s32(out, "edge0StartVert", v1);
				w_fp32(out, "edge0StartZ", getAttribute_float(tri, "edge0StartZ", z_array[v1]));
				w_s32(out, "edge0Connection", getAttribute_int(tri, "edge0Connection", -1));
				w_s32(out, "edge1StartVert", v2);
				w_fp32(out, "edge1StartZ", getAttribute_float(tri, "edge1StartZ", z_array[v2]));
				w_s32(out, "edge1Connection", getAttribute_int(tri, "edge1Connection", -1));
				w_s32(out, "edge2StartVert", v3);
				w_fp32(out, "edge2StartZ", getAttribute_float(tri, "edge2StartZ", z_array[v3]));
				w_s32(out, "edge2Connection", getAttribute_int(tri, "edge2Connection", -1));
				
				close_element(out, "tri");
			}
			
			close_element(out, "tris");
			
			free(z_array);
		
		close_element(out, "mesh3D");
		
		// mapping to 2D
		TOKNode *mapping2D = getNode(node, "mappingTo2D");
		open_element(out, "mappingTo2D");
		
			int nverts = 0;
			int npolys = 0;
			
			for(TOKNode *elem = mapping2D->child; elem; elem = elem->next)
			{
				if(strcmp(elem->name, "vert") == 0)
					nverts++;
				else if(strcmp(elem->name, "poly") == 0)
					npolys++;
			}
		
			// vertices
			open_element(out, "verts");
				assert(nverts == 0); // TODO
				w_s32(out, "size", 0);
			close_element(out, "verts");
			
			// polygons
			open_element(out, "polys");
			
			w_s32(out, "size", npolys);
			
			for(TOKNode *poly = mapping2D->child; poly; poly = poly->next)
			{
				if(strcmp(poly->name, "poly") == 0)
				{
					open_element(out, "poly");
					
					w_s32(out, "size", nodeCount(poly));
					
					for(TOKNode *edge = poly->child; edge; edge = edge->next)
					{
						open_element(out, "edge");
						w_s32(out, "startVert", getAttribute_int(edge, "startVert", -1));
						w_s32(out, "connection", getAttribute_int(edge, "connection", -1));
						close_element(out, "edge");
					}
					
					close_element(out, "poly");
				}
			}
			
			close_element(out, "polys");
		
		close_element(out, "mappingTo2D");
		
		// off mesh connections
		TOKNode *offmeshconnections = getNode(node, "offMeshConnections");
		
		open_element(out, "offMeshConnections");
		
		if(offmeshconnections)
		{
			// TODO not tested
			
			TOKNode *endpoints = getNode(offmeshconnections, "endPoints");
			open_element(out, "endPoints");
			
			w_s32(out, "size", nodeCount(endpoints));
			
			for(TOKNode *endpoint = endpoints->child; endpoint; endpoint = endpoint->next)
			{
				open_element(out, "endPoint");
				
				int f, x, y;
				const char *position = getAttribute_stringz(endpoint, "position", "0:0,0");
				
				sscanf(position, "%d:%d,%d", &f, &x, &y);
				
				open_element(out, "position");
				w_s32(out, "f", f);
				w_s32(out, "x", x);
				w_s32(out, "y", y);
				close_element(out, "position");
				
				close_element(out, "endPoint");
			}
			
			close_element(out, "endPoints");
			
			TOKNode *connections = getNode(offmeshconnections, "connections");
			open_element(out, "connections");
			
			w_s32(out, "size", nodeCount(connections));
			
			for(TOKNode *conn = connections->child; conn; conn = conn->next)
			{
				open_element(out, "connection");
				w_s32(out, "from",    getAttribute_int(conn, "from", -1));
				w_s32(out, "to",      getAttribute_int(conn, "to", -1));
				w_s32(out, "penalty", getAttribute_int(conn, "penalty", -1));
				close_element(out, "connection");
			} 
			
			close_element(out, "connections");
		}
		else
		{
			open_element(out, "endPoints");
			w_s32(out, "size", 0);
			close_element(out, "endPoints");
			
			open_element(out, "connections");
			w_s32(out, "size", 0);
			close_element(out, "connections");			
		}
		
		close_element(out, "offMeshConnections");
		
		// obstacles
		TOKNode *baseobstacles = getNode(node, "baseObstacles");
		
		open_element(out, "baseObstacles");
		w_s32(out, "size", 0); // TODO
		close_element(out, "baseObstacles");
		
		// surface types
		TOKNode *surfacetypes = getNode(node, "baseSurfaceTypeCosts");
		open_element(out, "baseSurfaceTypeCosts");
		
		if(surfacetypes)
		{
			w_s32(out, "size", nodeCount(surfacetypes));
			
			for(TOKNode *costs = surfacetypes->child; costs; costs = costs->next)
			{
				open_element(out, "costs");
				
				w_s32(out, "size", nodeCount(costs));
				
				for(TOKNode *entry = costs->child; entry; entry = entry->next)
				{
					open_element(out, "entry");
					
					w_s32(out, "surfaceType", getAttribute_int(entry, "surfaceType", 1));
					w_fp32(out, "cost", getAttribute_float(entry, "cost", 0.1));
					w_s32(out, "costDirectionX", getAttribute_int(entry, "costDirectionX", 0));
					w_s32(out, "costDirectionY", getAttribute_int(entry, "costDirectionY", 0));
					
					close_element(out, "entry");
				}
				
				close_element(out, "costs");
			}
		}
		else
			w_s32(out, "size", 0);
		
		close_element(out, "baseSurfaceTypeCosts");
		
	close_element(out, "mesh");
}

void convert_collisionPreprocess(MFILE *out, TOKNode *node)
{	
	mfwrite_char(0x10 | debug_info, out); // bin flags
	
	open_element(out, "collisionPreprocess");
	
		w_s32(out, "majorVersion", getAttribute_int(node, "majorVersion", -1));
		w_s32(out, "minorVersion", getAttribute_int(node, "minorVersion", -1));
		w_s32(out, "meshCheckSum", getAttribute_int(node, "meshCheckSum", -1));
		
		// shape
		TOKNode *shape = getNode(node, "shape");
		open_element(out, "shape");
		
			w_s32(out, "vertices", getAttribute_int(shape, "vertices", -1));
			
			for(TOKNode *vertex = shape->child; vertex; vertex = vertex->next)
			{
				open_element(out, "vertex");
				
					w_s32(out, "x", getAttribute_int(vertex, "x", -1));
					w_s32(out, "y", getAttribute_int(vertex, "y", -1));
				
				close_element(out, "vertex");
			}
		
		close_element(out, "shape");
		
		// edge expansion circuit, что бы это не значило
		TOKNode *ee = getNode(node, "edgeExpansion");
		open_element(out, "edgeExpansion");
		
		TOKNode *circuit = getNode(ee, "circuit");
		open_element(out, "circuit");
		
		w_s32(out, "size", nodeCount(circuit));
		
		for(TOKNode *cut = circuit->child; cut; cut = cut->next)
		{
			open_element(out, "cut");
			
			w_bool(out, "unconnected", getAttribute_int(cut, "unconnected", 0));
			w_s32(out, "type",         getAttribute_int(cut, "type", 0));
			w_s32(out, "start_face",   getAttribute_int(cut, "start_face", 0));
			w_s32(out, "size",         nodeCount(cut));
			
			for(TOKNode *element = cut->child; element; element = element->next)
			{
				const char *line = getAttribute_stringz(element, "line", "0,0,0,0");
				int sx, sy, ex, ey;
				
				sscanf(line, "%d,%d,%d,%d", &sx, &sy, &ex, &ey);
				
				open_element(out, "element");
				
				open_element(out, "line");
				w_s32(out, "sx", sx);
				w_s32(out, "sy", sy);
				w_s32(out, "ex", ex);
				w_s32(out, "ey", ey);
				close_element(out, "line");
				
				w_s32(out, "type", getAttribute_int(element, "type", 3)); // default value 3 ???
				
				close_element(out, "element");
			}
			
			close_element(out, "cut");
		}
		
		close_element(out, "circuit");
		close_element(out, "edgeExpansion");
	
	close_element(out, "collsinionPreprocess");
}

void convert_pathfindPreprocess(MFILE *out, TOKNode *node)
{
	mfwrite_char(0x10 | debug_info, out); // bin flags
	
	open_element(out, "pathfindPreprocess");
	
		w_s32(out, "majorVersion", getAttribute_int(node, "majorVersion", -1));
		w_s32(out, "minorVersion", getAttribute_int(node, "minorVersion", -1));
		w_s32(out, "meshCheckSum", getAttribute_int(node, "meshCheckSum", -1));
		
		// shape
		TOKNode *shape = getNode(node, "shape");
		open_element(out, "shape");
		
			w_s32(out, "vertices", getAttribute_int(shape, "vertices", -1));
			
			for(TOKNode *vertex = shape->child; vertex; vertex = vertex->next)
			{
				open_element(out, "vertex");
				
					w_s32(out, "x", getAttribute_int(vertex, "x", -1));
					w_s32(out, "y", getAttribute_int(vertex, "y", -1));
				
				close_element(out, "vertex");
			}
		
		close_element(out, "shape");
		
		// ?????????
		w_bool(out, "resolvePositionNearObstructedEndPoints", 0);
		
		// 
		TOKNode *pp = getNode(node, "preprocess");
		open_element(out, "preprocess");
		
			w_bool(out, "connectedRegionsPreprocess", 1);
		
			//
			TOKNode *ee = getNode(pp, "connectedRegionsPreprocess");
			open_element(out, "connectedRegionsPreprocess");
		
				// circuit какой-то
				TOKNode *circuit = getNode(ee, "circuit");
				open_element(out, "circuit");
				
				w_s32(out, "size", nodeCount(circuit));
				
				for(TOKNode *cut = circuit->child; cut; cut = cut->next)
				{
					open_element(out, "cut");
					
					w_bool(out, "unconnected", getAttribute_int(cut, "unconnected", 0));
					w_s32(out, "type",         getAttribute_int(cut, "type", 0));
					w_s32(out, "start_face",   getAttribute_int(cut, "start_face", 0));
					w_s32(out, "size",         nodeCount(cut));
					
					for(TOKNode *element = cut->child; element; element = element->next)
					{
						const char *line = getAttribute_stringz(element, "line", "0,0,0,0");
						int sx, sy, ex, ey;
						
						sscanf(line, "%d,%d,%d,%d", &sx, &sy, &ex, &ey);
						
						open_element(out, "element");
						
						open_element(out, "line");
						w_s32(out, "sx", sx);
						w_s32(out, "sy", sy);
						w_s32(out, "ex", ex);
						w_s32(out, "ey", ey);
						close_element(out, "line");
						
						w_s32(out, "type", getAttribute_int(element, "type", 3)); // default value 3 ???
						
						close_element(out, "element");
					}
					
					close_element(out, "cut");
				}
				
				close_element(out, "circuit");
		
				// region for element
				TOKNode *regionforelement = getNode(ee, "regionForElement");
				open_element(out, "regionForElement");
				
					w_s32(out, "size", nodeCount(regionforelement));
					for(TOKNode *entry = regionforelement->child; entry; entry = entry->next)
					{
						open_element(out, "entry");
						w_s32(out, "value", getAttribute_int(entry, "value", -1));
						close_element(out, "entry");
					}
				
				close_element(out, "regionForElement");

			close_element(out, "connectedRegionsPreprocess");
		
			// corners
			TOKNode *elementcorners = getNode(pp, "elementCorners");
			open_element(out, "elementCorners");
			
			w_s32(out, "numberOfHardCorners", getAttribute_int(elementcorners, "numberOfHardCorners", nodeCount(elementcorners)));
			w_s32(out, "size", nodeCount(elementcorners));
			
			for(TOKNode *corner = elementcorners->child; corner; corner = corner->next)
			{
				open_element(out, "corner");
				
				w_s32(out, "face", getAttribute_int(corner, "face", -1));
				w_s32(out, "x", getAttribute_int(corner, "x", -1));
				w_s32(out, "y", getAttribute_int(corner, "y", -1));
				w_s32(out, "axisBeforeX", getAttribute_int(corner, "axisBeforeX", -1));
				w_s32(out, "axisBeforeY", getAttribute_int(corner, "axisBeforeY", -1));
				w_s32(out, "axisAfterX", getAttribute_int(corner, "axisAfterX", -1));
				w_s32(out, "axisAfterY", getAttribute_int(corner, "axisAfterY", -1));
				
				close_element(out, "corner");
			}
			
			close_element(out, "elementCorners");
		
			// silhouette lookup v_v
			TOKNode *silhouettelookup = getNode(pp, "silhouetteLookup");
			open_element(out, "silhouetteLookup");
			
			int nnodes = 0;
			int nstates = 0;
			
			for(TOKNode *s = silhouettelookup->child; s; s = s->next)
			{
				nnodes += 1;
				nstates += nodeCount(s);
			}
			
			w_s32(out, "isize", nnodes); // number of 'pointsFromFace'/'statesFromFace' nodes
			w_s32(out, "asize", nstates); // number of 'point'/'state' entries
			
			for(TOKNode *n = silhouettelookup->child; n; n = n->next)
			{
				if(strcmp(n->name, "pointsFromFace") == 0)
				{
					open_element(out, "pointsFromFace");
					
					w_s32(out, "jsize", nodeCount(n));
					
					for(TOKNode *point = n->child; point; point = point->next)
					{
						open_element(out, "point");
						w_s32(out, "value", getAttribute_int(point, "value", 0));
						close_element(out, "point");
					}
					
					close_element(out, "pointsFromFace");
				}
				else if(strcmp(n->name, "statesFromFace") == 0)
				{
					open_element(out, "statesFromFace");
					
					w_s32(out, "jsize", nodeCount(n));
	
					for(TOKNode *state = n->child; state; state = state->next)
					{
						open_element(out, "state");
						w_u32(out, "value", getAttribute_int(state, "state", 0));
						close_element(out, "state");
					}
					
					close_element(out, "statesFromFace");
				}
				else
					assert(!"Unknown silhouetteLookup node");
			}
			
			close_element(out, "silhouetteLookup");
		
			// visibility graph
			TOKNode *visibilitygraph = getNode(pp, "visibilityGraph");
			open_element(out, "visibilityGraph");
			
			int nsource = 0;
			int ntarget = 0;
			
			for(TOKNode *source = visibilitygraph->child; source; source = source->next)
			{
				nsource += 1;
				ntarget += nodeCount(source); // or getAttribute_int(source, "size", 0) ?
			}
			
			w_s32(out, "asize", ntarget); // number iof 'target' nodes
			w_s32(out, "size", nsource); // number of 'source' nodes
			
			for(TOKNode *source = visibilitygraph->child; source; source = source->next)
			{
				w_s32(out, "jsize", nodeCount(source)); // or getAttribute_int(source, "size", 0) ?
				
				for(TOKNode *target = source->child; target; target = target->next)
				{
					w_u32(out, "state", getAttribute_int(target, "state", 0));
					w_u32(out, "cost", getAttribute_int(target, "cost", 0));
				}
			}
			
			close_element(out, "visibilityGraph");
		
		close_element(out, "preprocess");
	
	close_element(out, "pathfindPreprocess");
}

void convert_tok_to_bin(const void *tok_data, size_t tok_size, void **bin_data, size_t *bin_size, int _debug)
{
	MFILE *f;
	size_t size;
	
	TOKNode *node;
	
	MFILE *ground;
	MFILE *cp[3];
	MFILE *pfp[3];
	
	int i;
	int unknown0;
	float radius[3];
	
	int sign_mesh      = 0x7B5C7E87; // crc32("ground_ver0001");
	int sign_collision = 0x08DDF1EE; // crc32("mesh_preprocess");
	int sign_pathfind  = 0x350334F3; // crc32("os_preprocess");
	
	debug_info = _debug;
	
	f = mfopen(tok_data, tok_size);
	
	// 1. ground model
	size = mfread_int(f);

	printf("converting ground...\n");
	ground = mfcreate(256 * 1024);
	
	node = parse_tok((char*)f->buffer + f->cursor, size); f->cursor += size;
	convert_mesh3D(ground, node);
	free_tok(node);
	
	// 2. collisionPreprocess & pathfindPreprocess
	unknown0 = mfread_int(f);
	
	for(i = 0; i < 3; i++)
	{
		radius[i] = mfread_float(f);
		
		// collision
		size = mfread_int(f);
		
		printf("converting collisionPreprocess%d...\n", i+1);
		cp[i] = mfcreate(256 * 1024);
		
		node = parse_tok((char*)f->buffer + f->cursor, size);	f->cursor += size;
		convert_collisionPreprocess(cp[i], node);
		free_tok(node);
		
		// pathfind
		size = mfread_int(f);
		
		printf("converting pathfindPreprocess%d...\n", i+1);
		pfp[i] = mfcreate(256 * 1024);
		
		node = parse_tok((char*)f->buffer + f->cursor, size);	f->cursor += size;
		convert_pathfindPreprocess(pfp[i], node);
		free_tok(node);
	}
	
	mfclose(f);
	
	// 3. MERGE !!!!
	
	f = mfcreate(256 * 1024);
	
	mfwrite_int(sign_mesh, f);
	mfwrite_int(ground->length, f);
	mfwrite(ground->buffer, ground->length, f);
	
	mfwrite_int(unknown0, f);
	
	for(i = 0; i < 3; i++)
	{
		mfwrite_float(radius[i], f);
		mfwrite_int(sign_collision, f);
		mfwrite_int(cp[i]->length + pfp[i]->length, f);
		
		mfwrite(cp[i]->buffer, cp[i]->length, f);
		mfwrite(pfp[i]->buffer, pfp[i]->length, f);
	}
	
	mfwrite_int(unknown0, f);
	
	for(i = 0; i < 3; i++)
	{
		mfwrite_float(radius[i], f);
		mfwrite_int(sign_pathfind, f);
		
		mfwrite_int(pfp[i]->length, f);
		mfwrite(pfp[i]->buffer, pfp[i]->length, f);
	}
	
	// finita la comedia
	*bin_data = f->buffer;
	*bin_size = f->length;
	
	f->buffer = NULL;
	mfclose(f);

	for(i = 0; i < 3; i++)
	{
		mfclose(cp[i]);
		mfclose(pfp[i]);
	}
	
	mfclose(ground);
}

