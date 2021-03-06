﻿#include "RoadMeshGenerator.h"
#include "Util.h"
#include "global.h"

bool compare2ndPartTuple2 (const std::pair<float, RoadEdgeDesc> &i, const std::pair<float, RoadEdgeDesc> &j) {
	return (i.first > j.first);
}

void RoadMeshGenerator::generateRoadMesh(VBORenderManager& rendManager, RoadGraph& roads) {
	float deltaZ = 2.0f;

	std::vector<Vertex> vertSide;

	//////////////////////////////////////////
	// POLYLINES
	{
		float const maxSegmentLeng = 5.0f;

		RoadEdgeIter ei, eiEnd;
		int numEdges = 0;

		std::vector<Vertex> vertROAD[2];
		std::vector<Vertex> intersectCirclesV;
		for (boost::tie(ei, eiEnd) = boost::edges(roads.graph); ei != eiEnd; ++ei) {
			if (!roads.graph[*ei]->valid) continue;
			numEdges++;

			RoadEdgePtr edge = roads.graph[*ei];
			float hWidth;
			if (roads.graph[*ei]->type != RoadEdge::TYPE_STREET) {
				hWidth = G::getFloat("major_road_width");
			}
			else {
				hWidth = G::getFloat("minor_road_width");
			}
			
			int type;
			switch (roads.graph[*ei]->type) {
			case RoadEdge::TYPE_HIGHWAY:
			case RoadEdge::TYPE_BOULEVARD:
			case RoadEdge::TYPE_AVENUE:
				type = 1;
				break;
			case RoadEdge::TYPE_STREET:
				type = 0;
				break;
			default:
				type = 0;
				break;
			}
			
			float lengthMovedL = 0; // road texture dX
			float lengthMovedR = 0; // road texture dX

			QVector2D a0,a1,a2,a3;

			for (int pL = 0; pL < edge->polyline.size() - 1; pL++) {
				bool bigAngle = false;
				QVector2D p0 = edge->polyline[pL];
				QVector2D p1 = edge->polyline[pL+1];
				if ((p0 - p1).lengthSquared() <= 0.00001f) continue;

				QVector2D dir = p1 -p0;
				float length = dir.length();
				dir /= length;
				
				QVector2D per(-dir.y(), dir.x());
				if (pL == 0) {
					a0 = p0 - per * hWidth;
					a3 = p0 + per * hWidth;
				}
				a1 = p1 - per * hWidth;
				a2 = p1 + per * hWidth;

				QVector2D p2;
				if (pL < edge->polyline.size() - 2) {
					p2 = edge->polyline[pL + 2];
					
					// Use getIrregularBisector only if the two segments are not parallel.
					if (fabs(QVector2D::dotProduct((p2 - p1).normalized(), (p1 - p0).normalized())) < 0.99f) {
						Util::getIrregularBisector(p0, p1, p2, hWidth, hWidth, a2);
						Util::getIrregularBisector(p0, p1, p2, -hWidth, -hWidth, a1);
					}
				}
				
				float middLenghtR = length;
				float middLenghtL = length;
				float segmentLengR, segmentLengL;
				int numSegments = ceil(length/5.0f);

				float dW = 7.5f;//tex size in m

				QVector3D b0, b3;
				QVector3D b1 = a0;
				QVector3D b2 = a3;
				QVector3D vecR = a1 - a0;
				QVector3D vecL = a2 - a3;

				for(int nS=0;nS<numSegments;nS++){
					segmentLengR = std::min(maxSegmentLeng, middLenghtR);
					segmentLengL = std::min(maxSegmentLeng, middLenghtL);

					b0 = b1;
					b3 = b2;
					if (nS < numSegments - 1) {
						b1 += dir * segmentLengR;
						b2 += dir * segmentLengL;
					} else {
						b1 = a1;
						b2 = a2;
					}
									

					QVector3D b03 = (b0 + b3) * 0.5f;
					QVector3D b12 = (b1 + b2) * 0.5f;
					float z1 = rendManager.getTerrainHeight(b03.x(), b03.y());
					if (z1 < G::getFloat("road_min_level")) z1 = G::getFloat("road_min_level");
					float z2 = rendManager.getTerrainHeight(b12.x(), b12.y());
					if (z2 < G::getFloat("road_min_level")) z2 = G::getFloat("road_min_level");

					b0.setZ(z1 + deltaZ);
					b3.setZ(z1 + deltaZ);
					b1.setZ(z2 + deltaZ);
					b2.setZ(z2 + deltaZ);

					vertROAD[type].push_back(Vertex(b0, QColor(), QVector3D(0, 0, 1.0f), QVector3D(1, lengthMovedR / dW, 0)));
					vertROAD[type].push_back(Vertex(b1, QColor(), QVector3D(0, 0, 1.0f), QVector3D(1, (lengthMovedR + segmentLengR) / dW, 0)));
					vertROAD[type].push_back(Vertex(b2, QColor(), QVector3D(0, 0, 1.0f), QVector3D(0, (lengthMovedL + segmentLengL) / dW, 0)));
					vertROAD[type].push_back(Vertex(b3, QColor(), QVector3D(0, 0, 1.0f), QVector3D(0, lengthMovedL / dW, 0)));

					// side face geometry
					vertSide.push_back(Vertex(b0 + QVector3D(0, 0, -deltaZ * 2), QColor(64, 64, 64), -per, QVector3D()));
					vertSide.push_back(Vertex(b1 + QVector3D(0, 0, -deltaZ * 2), QColor(64, 64, 64), -per, QVector3D()));
					vertSide.push_back(Vertex(b1, QColor(64, 64, 64), -per, QVector3D()));
					vertSide.push_back(Vertex(b0, QColor(64, 64, 64), -per, QVector3D()));

					vertSide.push_back(Vertex(b2 + QVector3D(0, 0, -deltaZ * 2), QColor(64, 64, 64), per, QVector3D()));
					vertSide.push_back(Vertex(b3 + QVector3D(0, 0, -deltaZ * 2), QColor(64, 64, 64), per, QVector3D()));
					vertSide.push_back(Vertex(b3, QColor(64, 64, 64), per, QVector3D()));
					vertSide.push_back(Vertex(b2, QColor(64, 64, 64), per, QVector3D()));

					lengthMovedR += segmentLengR;
					lengthMovedL += segmentLengL;
					middLenghtR -= segmentLengR;
					middLenghtL -= segmentLengL;
				}				

				a3 = a2;
				a0 = a1;
			}
		}
		
		// add all geometry
		rendManager.addStaticGeometry("3d_roads", vertROAD[0], "data/textures/roads/road_2lines.jpg", GL_QUADS, 2);
		rendManager.addStaticGeometry("3d_roads", vertROAD[1], "data/textures/roads/road_4lines.jpg", GL_QUADS, 2);
	}

	//////////////////////////////////////////
	// Circle+Complex
	{
		// 2. INTERSECTIONS
		std::vector<Vertex> intersectCirclesV;
		std::vector<Vertex> interPedX;
		std::vector<Vertex> interPedXLineR;

		RoadVertexIter vi, vend;
		for (boost::tie(vi, vend) = boost::vertices(roads.graph); vi != vend; ++vi) {
			if (!roads.graph[*vi]->valid) continue;

			int outDegree = roads.getDegree(*vi);

			if (outDegree == 0) {
				continue;
			} else if (outDegree == 1) { // dead end
				// get the largest width of the outing edges
				float rad = 0.0f;
				float angle_offset = 0.0f;
				RoadOutEdgeIter oei, oeend;
				for (boost::tie(oei, oeend) = boost::out_edges(*vi, roads.graph); oei != oeend; ++oei) {
					if (!roads.graph[*oei]->valid) continue;

					if (roads.graph[*oei]->type != RoadEdge::TYPE_STREET) {
						rad = G::getFloat("major_road_width");
					}
					else {
						rad = G::getFloat("minor_road_width");
					}
					Polyline2D polyline = roads.orderPolyLine(*oei, *vi);
					QVector2D dir = polyline[1] - polyline[0];
					angle_offset = atan2f(dir.x(), -dir.y());
					break;
				}

				float z = rendManager.getTerrainHeight(roads.graph[*vi]->pt.x(), roads.graph[*vi]->pt.y());
				if (z < G::getFloat("road_min_level")) z = G::getFloat("road_min_level");
				QVector3D center(roads.graph[*vi]->pt.x(), roads.graph[*vi]->pt.y(), z + deltaZ);

				const float numSides = 10;
				QVector3D cc1 = QVector3D(rad * cosf(angle_offset), rad * sinf(angle_offset), 0.0f);
				for (int i = 1; i <= numSides; ++i) {
					float angle = angle_offset + M_PI * i / numSides;
					QVector3D cc2 = QVector3D(rad * cosf(angle), rad * sinf(angle), 0.0f);

					intersectCirclesV.push_back(Vertex(center, center/7.5f));
					intersectCirclesV.push_back(Vertex(center + cc1, (center + cc1) / 7.5f));
					intersectCirclesV.push_back(Vertex(center + cc2, (center + cc2) / 7.5f));

					// side face geometry
					QVector3D side0_u = center + cc1;
					QVector3D side0_b = center + cc1 + QVector3D(0, 0, -deltaZ * 2.0f);
					QVector3D side1_u = center + cc2;
					QVector3D side1_b = center + cc2 + QVector3D(0, 0, -deltaZ * 2.0f);

					vertSide.push_back(Vertex(side0_b, QColor(64, 64, 64), cc1, QVector3D()));
					vertSide.push_back(Vertex(side1_b, QColor(64, 64, 64), cc2, QVector3D()));
					vertSide.push_back(Vertex(side1_u, QColor(64, 64, 64), cc2, QVector3D()));
					vertSide.push_back(Vertex(side0_u, QColor(64, 64, 64), cc1, QVector3D()));

					cc1 = cc2;
				}
			} else {
				////////////////////////
				// 2.2 FOUR OR MORE--> COMPLEX INTERSECTION
				float z = rendManager.getTerrainHeight(roads.graph[*vi]->pt.x(), roads.graph[*vi]->pt.y());
				if (z < G::getFloat("road_min_level")) z = G::getFloat("road_min_level");
				z += deltaZ + 0.1f;

				////////////
				// 2.2.1 For each vertex find edges and sort them in clockwise order
				std::vector<std::pair<float, RoadEdgeDesc> > edgeAngleOut;
				RoadOutEdgeIter Oei, Oei_end;
				QMap<RoadEdgeDesc, bool> visited;
				//printf("a1\n");
				for (boost::tie(Oei, Oei_end) = boost::out_edges(*vi, roads.graph); Oei != Oei_end; ++Oei) {
					if (!roads.graph[*Oei]->valid) continue;
					if (visited[*Oei]) continue;

					// GEN 1/12/2015
					// to avoid some garbage in the boost graph
					RoadVertexDesc tgt = boost::target(*Oei, roads.graph);
					if (*vi == 0 && *vi == tgt) continue;

					Polyline2D polyline = roads.orderPolyLine(*Oei, *vi);
					QVector2D p0 = polyline[0];
					QVector2D p1 = polyline[1];

					QVector2D edgeDir=(p1-p0).normalized();// NOTE p1-p0

					float angle = atan2(edgeDir.y(),edgeDir.x());
					edgeAngleOut.push_back(std::make_pair(angle, *Oei));//z as width

					// For self-loop edge
					if (tgt == *vi) {
						p0 = polyline.back();
						p1 = polyline[polyline.size() - 2];
						edgeDir = (p1 - p0).normalized();
						float angle = atan2(edgeDir.y(),edgeDir.x());
						edgeAngleOut.push_back(std::make_pair(angle, *Oei));//z as width
					}

					visited[*Oei] = true;
				}
				std::sort(edgeAngleOut.begin(), edgeAngleOut.end(), compare2ndPartTuple2);

				// 2.2.2 Create intersection geometry of the given edges
				std::vector<QVector3D> interPoints;
				std::vector<QVector3D> stopPoints;
				float prev_angle = std::numeric_limits<float>::max();
				for (int eN = 0; eN < edgeAngleOut.size(); eN++) {
					// GEN
					// Some OSM files have redundant edges. To handle this situation,
					// skip the edge if its angle is the same as the previous one.
					// This is just a HACK, so it is recommended to clean up the roads.
					if (edgeAngleOut[eN].first == prev_angle) continue;
					prev_angle = edgeAngleOut[eN].first;

					//printf("** eN %d\n",eN);
					// a) ED1: this edge
					float ed1W;
					if (roads.graph[edgeAngleOut[eN].second]->type != RoadEdge::TYPE_STREET) {
						ed1W = G::getFloat("major_road_width");
					}
					else {
						ed1W = G::getFloat("minor_road_width");
					}
					Polyline2D ed1poly = roads.orderPolyLine(edgeAngleOut[eN].second, *vi);// , edgeAngleOut[eN].first);
					QVector2D ed1p1 = ed1poly[1];
					// compute right side
					QVector2D ed1Dir = (roads.graph[*vi]->pt - ed1p1).normalized();//ends in 0
					QVector2D ed1Per(ed1Dir.y(), -ed1Dir.x());
					QVector2D ed1p0R = roads.graph[*vi]->pt + ed1Per * ed1W;
					QVector2D ed1p1R = ed1p1 + ed1Per * ed1W;
					// compute left side
					QVector2D ed1p0L = roads.graph[*vi]->pt - ed1Per * ed1W;
					QVector2D ed1p1L = ed1p1 - ed1Per * ed1W;

					// b) ED2: next edge
					int lastEdge = eN - 1;
					if (lastEdge < 0) lastEdge = edgeAngleOut.size() - 1;
					float ed2WL;
					if (roads.graph[edgeAngleOut[lastEdge].second]->type != RoadEdge::TYPE_STREET) {
						ed2WL = G::getFloat("major_road_width");
					}
					else {
						ed2WL = G::getFloat("minor_road_width");
					}
					QVector2D ed2p0L = roads.graph[*vi]->pt;
					Polyline2D ed2polyL = roads.orderPolyLine(edgeAngleOut[lastEdge].second, *vi);// , edgeAngleOut[lastEdge].first);
					QVector2D ed2p1L = ed2polyL[1];
					// compute left side
					QVector2D ed2DirL = (ed2p0L - ed2p1L).normalized();//ends in 0
					QVector2D ed2PerL(ed2DirL.y(), -ed2DirL.x());
					ed2p0L -= ed2PerL * ed2WL;
					ed2p1L -= ed2PerL * ed2WL;

					// c) ED2: last edge
					int nextEdge = (eN + 1) % edgeAngleOut.size();
					float ed2WR;
					if (roads.graph[edgeAngleOut[nextEdge].second]->type != RoadEdge::TYPE_STREET) {
						ed2WR = G::getFloat("major_road_width");
					}
					else {
						ed2WR = G::getFloat("minor_road_width");
					}
					QVector2D ed2p0R = roads.graph[*vi]->pt;
					Polyline2D ed2polyR = roads.orderPolyLine(edgeAngleOut[nextEdge].second, *vi);// , edgeAngleOut[nextEdge].first);
					QVector2D ed2p1R = ed2polyR[1];
					// compute left side
					QVector2D ed2DirR = (ed2p0R - ed2p1R).normalized();//ends in 0
					QVector2D ed2PerR(ed2DirR.y(), -ed2DirR.x());
					ed2p0R += ed2PerR * ed2WR;
					ed2p1R += ed2PerR * ed2WR;

					//////////////////////////////////////////
					// d) Computer interior coordinates
					// d.1 computer intersection left
					QVector2D intPt1(FLT_MAX,0);
					if (fabs(QVector2D::dotProduct(ed1Dir, ed2DirL)) < 0.95f) { // adjacent road segments are not parallel
						double tab,tcd;
						Util::segmentSegmentIntersectXY(ed1p0R, ed1p1R, ed2p0L, ed2p1L, &tab, &tcd, false, intPt1);
					} else { // adjacent road segments are parallel
						intPt1 = (ed1p0R + ed2p0L) * 0.5f;
					}
					// d.2 computer intersecion right
					QVector2D intPt2(FLT_MAX,0);
					if (fabs(QVector2D::dotProduct(ed1Dir, ed2DirR)) < 0.95f) {
						double tab, tcd;
						Util::segmentSegmentIntersectXY(ed1p0L, ed1p1L, ed2p0R, ed2p1R, &tab, &tcd, false, intPt2);
					} else {
						intPt2 = (ed1p0L + ed2p0R) * 0.5f;
					}

					QVector3D intPoint1(intPt1.x(), intPt1.y(), z);
					QVector3D intPoint2(intPt2.x(), intPt2.y(), z);

					interPoints.push_back(intPoint1);
					// align intPoint1 and intPoint2 such that they are perpendicular to the road orientation
					if (QVector3D::dotProduct(intPoint1 - intPoint2, ed1Dir) >= 0) {
						intPoint1 -= ed1Dir * QVector3D::dotProduct(intPoint1 - intPoint2, ed1Dir);
						interPoints.push_back(intPoint1);
					} else {
						intPoint2 += ed1Dir * QVector3D::dotProduct(intPoint1 - intPoint2, ed1Dir);
						interPoints.push_back(intPoint2);
					}
					
					stopPoints.push_back(intPoint1);
					stopPoints.push_back(intPoint2);

					if (outDegree >= 3 && roads.graph[edgeAngleOut[eN].second]->type != RoadEdge::TYPE_STREET && (ed1poly[0] - ed1poly.back()).length() > 10.0f && ed1poly.length() > 50.0f) {
						// crosswalk
						interPedX.push_back(Vertex(intPoint1, QVector3D(0 - 0.07f, 0, 0)));
						interPedX.push_back(Vertex(intPoint2, QVector3D(ed1W / 7.5f + 0.07f, 0, 0)));
						interPedX.push_back(Vertex(intPoint2 - ed1Dir * 3.5f, QVector3D(ed1W / 7.5f + 0.07f, 1.0f, 0)));
						interPedX.push_back(Vertex(intPoint1 - ed1Dir * 3.5f, QVector3D(0.0f - 0.07f, 1.0f, 0)));

						// stop line
						QVector3D midPoint=(intPoint2+intPoint1)/2.0f+0.2f*ed1Per;
					
						interPedXLineR.push_back(Vertex(intPoint1 - ed1Dir * 3.5f, QVector3D(0, 0.0f, 0)));
						interPedXLineR.push_back(Vertex(midPoint - ed1Dir * 3.5f, QVector3D(1.0f, 0.0f, 0)));
						interPedXLineR.push_back(Vertex(midPoint - ed1Dir * 4.25f, QVector3D(1.0f, 1.0f, 0)));
						interPedXLineR.push_back(Vertex(intPoint1 - ed1Dir * 4.25f, QVector3D(0.0f, 1.0f, 0)));
					}
				}
								
				if (interPoints.size() > 2) {
					rendManager.addStaticGeometry2("3d_roads", interPoints,0.0f, "data/textures/roads/road_0lines.jpg", 2, QVector3D(1.0f / 7.5f, 1.0f / 7.5f, 1), QColor());
				}
			}
		}

		rendManager.addStaticGeometry("3d_roads", intersectCirclesV, "data/textures/roads/road_0lines.jpg", GL_TRIANGLES, 2);
		rendManager.addStaticGeometry("3d_roads", interPedX, "data/textures/roads/road_pedX.jpg", GL_QUADS, 2);
		rendManager.addStaticGeometry("3d_roads", interPedXLineR, "data/textures/roads/road_pedXLineR.jpg", GL_QUADS, 2);
	}

	rendManager.addStaticGeometry("3d_roads", vertSide, "", GL_QUADS, 1|mode_Lighting);
}
