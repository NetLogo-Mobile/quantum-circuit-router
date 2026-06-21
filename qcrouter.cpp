#include <stdio.h>
#include "qcrouter.h"
#include "json.hpp"
#include "cola.h"
#include "aca3.h"
#include "assertions.h"
#include "orthogonal_topology.h"
#include "cola_topology_addon.h"
using Json = nlohmann::json;

extern "C" {
	// General-purpose interface
	/*Utility function ReturnJSON: returns the JSON as a string.*/
	int ReturnJSON(Json Json, char* Output) {
		std::string Dump = Json.dump();
		Dump.copy(Output, Dump.length(), 0);
		return Dump.length();
	}
	/*Utility function ReadJSON: parses JSON from a string.*/
	Json ReadJSON(char* Input) {
		return Json::parse(Input);
	}
	/*HelloWorld: just here to make you smile.*/
	CIVITAS_EXPORT const int HelloWorld(char* Input, char* Output) {
		Json Result = ReadJSON(Input);
		Result["hello"] = "world";
		return ReturnJSON(Result, Output);
	}

	// Router management
	std::vector<Avoid::Router*> Routers;
	std::vector<ShapeSet*> Shapes;
	std::vector<ConnectorSet*> Connectors;
	/*RegisterRouter: registers a new router.*/
	CIVITAS_EXPORT const int RegisterRouter() {
		auto Router = new Avoid::Router(Avoid::RouterFlag::OrthogonalRouting);
		// Configure options
		//Router->setRoutingOption(Avoid::RoutingOption::nudgeOrthogonalSegmentsConnectedToShapes, true);
		Router->setRoutingOption(Avoid::RoutingOption::nudgeOrthogonalTouchingColinearSegments, true);
		Router->setRoutingOption(Avoid::RoutingOption::nudgeSharedPathsWithCommonEndPoint, false);
		Router->setRoutingOption(Avoid::RoutingOption::performUnifyingNudgingPreprocessingStep, true);
		Router->setRoutingOption(Avoid::RoutingOption::penaliseOrthogonalSharedPathsAtConnEnds, false);
		Router->setRoutingOption(Avoid::RoutingOption::improveHyperedgeRoutesMovingJunctions, false);
		Router->setRoutingOption(Avoid::RoutingOption::improveHyperedgeRoutesMovingAddingAndDeletingJunctions, false);
		Router->setRoutingParameter(Avoid::RoutingParameter::shapeBufferDistance, 50);
		Router->setRoutingParameter(Avoid::RoutingParameter::idealNudgingDistance, 100);
		Router->setRoutingParameter(Avoid::RoutingParameter::segmentPenalty, 100);
		Router->setRoutingParameter(Avoid::RoutingParameter::crossingPenalty, 1000);
		Router->setRoutingParameter(Avoid::RoutingParameter::fixedSharedPathPenalty, 100000);
		Router->setRoutingParameter(Avoid::RoutingParameter::reverseDirectionPenalty, 50);
		Router->setTransactionUse(true);
		// Look for a free slot
		for (int i = 0; i < Routers.size(); i++)
			if (Routers[i] == NULL) {
				Routers[i] = Router;
				Shapes[i] = new ShapeSet();
				Connectors[i] = new ConnectorSet();
				return i;
			}
		// Append a new entry
		Routers.push_back(Router);
		Shapes.push_back(new ShapeSet());
		Connectors.push_back(new ConnectorSet());
		return Routers.size() - 1;
	}
	/*UnregisterRouter: unregisters a router.*/
	CIVITAS_EXPORT const void UnregisterRouter(int ID) {
		Routers[ID] = NULL;
		Shapes[ID] = NULL;
		Connectors[ID] = NULL;
	}
	/*FindVertex: finds the vertex with the given id.*/
	const int FindVertex(ShapeSet& RVertexes, unsigned int ID) {
		for (int I = 0; I < RVertexes.size(); I++)
			if (RVertexes[I] != NULL && RVertexes[I]->id() == ID)
				return I;
		return -1;
	};
	/*FindEdge: finds the connector with the given id.*/
	const int FindEdge(ConnectorSet& REdges, unsigned int ID) {
		for (int I = 0; I < REdges.size(); I++)
			if ((intptr_t)REdges[I] != NULL && REdges[I]->id() == ID)
				return I;
		return -1;
	};
	/*GetPinFlag: gets the direction of the given pin.*/
	const Avoid::ConnDirFlag GetPinFlag(double SizeX, double SizeY, double PinX, double PinY) {
		auto Flags = Avoid::ConnDirFlag::ConnDirAll;
		if (PinX == SizeX / 2) Flags = Avoid::ConnDirFlag::ConnDirRight; // On the right
		if (PinX == -SizeX / 2) Flags = Avoid::ConnDirFlag::ConnDirLeft; // On the left
		if (PinY == SizeY / 2) Flags = Avoid::ConnDirFlag::ConnDirDown; // On the top
		if (PinY == -SizeY / 2) Flags = Avoid::ConnDirFlag::ConnDirUp; // On the bottom
		return Flags;
	}
	/*CreatePins: creates all pins.*/
	void CreatePins(nlohmann::basic_json<> & Vertex, double SizeX, double SizeY, Avoid::ShapeRef * Shape) {
		int I = 1;
		for (auto& Pin : Vertex["Pins"]) {
			double X = Pin["X"]; double Y = Pin["Y"];
			int Flags = GetPinFlag(SizeX, SizeY, X, Y);
			auto Node = new Avoid::ShapeConnectionPin(Shape,
				I, X / SizeX + 0.5, Y / SizeY + 0.5, true, 0, Flags);
			Node->setExclusive(false); I++;
		}
	}
	/*ImportChanges: imports the changes before layout.*/
	const int ImportChanges(int ID, char* Input) {
		// Read the data
		auto& Router = Routers[ID];
		auto& RVertexes = *Shapes[ID];
		auto& REdges = *Connectors[ID];
		auto Modifies = ReadJSON(Input);
		int Changes = 0;
		// Read the nodes
		auto& Vertexes = Modifies["Vertexes"];
		for (auto& Vertex : Vertexes) {
			// Read the fields
			int ID = Vertex["ID"];
			int Status = Vertex["Status"];
			double PositionX = Vertex["Position"]["X"];
			double PositionY = Vertex["Position"]["Y"];
			double SizeX = Vertex["Size"]["X"];
			double SizeY = Vertex["Size"]["Y"];
			// Handle by case
			switch (Status) {
			case 1: { // Modified
				auto VertexID = FindVertex(RVertexes, ID);
				if (VertexID == -1) break;
				// Adjust position
				auto RVertex = RVertexes[VertexID]; int I = 1;
				auto Center = new Avoid::Point(PositionX, PositionY);
				auto Rectangle = new Avoid::Rectangle(*Center, SizeX, SizeY);
				Router->moveShape(RVertex, *Rectangle);
				// Adjust pins
				for (auto& Pin : Vertex["Pins"]) {
					double X = Pin["X"]; double Y = Pin["Y"];
					auto Flags = GetPinFlag(SizeX, SizeY, X, Y);
					for (auto& RPin : RVertex->attachedPins()) {
						if (RPin->ids().second != I) continue;
						RPin->updatePositionAndVisibility(X / SizeX + 0.5, Y / SizeY + 0.5, Flags);
					} I++;
				}
				// Cleanup
				delete Center; delete Rectangle;
				Changes++; break;
			}
			case 2: { // Added
				auto Center = new Avoid::Point(PositionX, PositionY);
				auto Rectangle = new Avoid::Rectangle(*Center, SizeX, SizeY);
				auto Shape = new Avoid::ShapeRef(Router, *Rectangle, ID);
				CreatePins(Vertex, SizeX, SizeY, Shape);
				bool EmptyFound = false;
				for (int I = 0; I < RVertexes.size(); I++)
					if (RVertexes[I] == NULL) {
						RVertexes[I] = Shape;
						EmptyFound = true;
						break;
					}
				if (!EmptyFound) RVertexes.push_back(Shape);
				delete Center; delete Rectangle;
				Changes++; break;
			}
			case 3: { // Removed
				auto VertexID = FindVertex(RVertexes, ID);
				if (VertexID == -1) break;
				auto& Vertex = RVertexes[VertexID];
				Router->deleteShape(Vertex);
				RVertexes[VertexID] = NULL;
				Changes++; break;
			}
			}
		}
		// Read the edges
		auto& Edges = Modifies["Edges"];
		for (auto& Edge : Edges) {
			// Read the fields
			int ID = Edge["ID"];
			int Status = Edge["Status"];
			int Source = Edge["Source"];
			int Target = Edge["Target"];
			int SourcePin = (int)Edge["SourcePin"] + 1;
			int TargetPin = (int)Edge["TargetPin"] + 1;
			// Handle by case
			switch (Status) {
			case 2: { // Added
				auto SourceNode = RVertexes[FindVertex(RVertexes, Source)];
				auto TargetNode = RVertexes[FindVertex(RVertexes, Target)];
				auto SourcePoint = Avoid::ConnEnd(SourceNode, SourcePin);
				auto TargetPoint = Avoid::ConnEnd(TargetNode, TargetPin);
				auto Connector = new Avoid::ConnRef(Router, SourcePoint, TargetPoint, ID);
				bool EmptyFound = false;
				Connector->source = SourceNode;
				Connector->destination = TargetNode;
				Connector->sourcePin = SourcePin;
				Connector->destinationPin = TargetPin;
				Changes++;
				for (int I = 0; I < REdges.size(); I++)
					if (REdges[I] == NULL) {
						REdges[I] = Connector;
						EmptyFound = true;
						break;
					}
				if (!EmptyFound) REdges.push_back(Connector);
				break;
			}
			case 3: { // Removed
				auto EdgeID = FindEdge(REdges, ID);
				if (EdgeID == -1) break;
				auto& Edge = REdges[EdgeID];
				Router->deleteConnector(Edge);
				REdges[EdgeID] = NULL;
				Changes++; break;
			}
			}
		}
		// Return the number of changes
		return Changes;
	}
	/*ExportChanges: exports the changes after layout.*/
	void ExportChanges(Json &Result, ShapeSet &RVertexes, ConnectorSet &REdges) {
		int N = 0;
		for (int M = 0; M < RVertexes.size(); M++) {
			auto Vertex = RVertexes[M];
			if ((intptr_t)Vertex == NULL) continue;
			auto& Node = Result["Nodes"][N];
			Node["ID"] = Vertex->id();
			auto X = Vertex->position().x;
			auto Y = Vertex->position().y;
			Node["X"] = X; Node["Y"] = Y; N++;
		}
		Result["Statistics"]["NodesChanged"] = N; N = 0;
		for (int M = 0; M < REdges.size(); M++) {
			auto Edge = REdges[M];
			if ((intptr_t)Edge == NULL) continue;
			auto& Node = Result["Edges"][N];
			auto Route = Edge->displayRoute().ps;
			Node["ID"] = Edge->id();
			for (int X = 0; X < Route.size(); X++) {
				auto& Point = Node["Routes"][X];
				Point["X"] = Route[X].x;
				Point["Y"] = Route[X].y;
			} N++;
		}
		Result["Statistics"]["EdgesChanged"] = N;
	}
	/*MapRectangles: maps Avoid shapes to Cola.*/
	void MapRectangles(ShapeSet & RVertexes, cola::VariableIDMap &RectangleMap, vpsc::Rectangles &Rectangles, cola::RootCluster &Cluster) {
		for (int M = 0; M < RVertexes.size(); M++) {
			auto Box = RVertexes[M]->polygon().offsetBoundingBox(0);
			auto Rectangle = new vpsc::Rectangle(Box.min.x, Box.max.x, Box.min.y, Box.max.y, false);
			RectangleMap.addMappingForVariable(Rectangles.size(), RVertexes[M]->id());
			Cluster.addChildNode(Rectangles.size());
			Rectangles.push_back(Rectangle);
		}
	}
	/*CalculateLayout: computes the component layout of the given circuit.*/
	CIVITAS_EXPORT const int CalculateLayout(int ID, char* Input, char* Output) {
		Json Result; int Changes = ImportChanges(ID, Input);
		// No changes
		if (Changes == 0) return ReturnJSON(Result, Output);
		// Read the data
		auto& Router = Routers[ID];
		auto& RVertexes = *Shapes[ID];
		auto& REdges = *Connectors[ID];
		auto Modifies = ReadJSON(Input);
		// Layout support
		auto RectangleMap = cola::VariableIDMap();
		auto Rectangles = std::vector<vpsc::Rectangle*>();
		auto Cluster = cola::RootCluster();
		auto Constraints = cola::CompoundConstraints();
		// Layout helpers
		MapRectangles(RVertexes, RectangleMap, Rectangles, Cluster);
		// Build the edges
		auto CEdges = std::vector<cola::Edge>();
		for (int M = 0; M < REdges.size(); M++) {
			auto CEdge1 = cola::Edge();
			auto CEdge2 = cola::Edge();
			if ((intptr_t)REdges[M] == NULL) continue;
			auto SourceID = REdges[M]->source->id();
			auto TargetID = REdges[M]->destination->id();
			CEdge1.first = RectangleMap.mappingForVariable(SourceID, false);
			CEdge2.first = RectangleMap.mappingForVariable(TargetID, false);
			CEdge1.second = RectangleMap.mappingForVariable(TargetID, false);
			CEdge2.second = RectangleMap.mappingForVariable(SourceID, false);
			CEdges.push_back(CEdge1);
			CEdges.push_back(CEdge2);
		}
		// Run the layout
		try {
			auto Layout = new cola::ACALayout3(Rectangles, CEdges, Constraints, 800, true);
			Layout->doFinalLayout(true);
			Layout->layout();
			Result["Succeeded"] = true;
			for (int M = 0; M < Rectangles.size(); M++) {
				auto& Rectangle = Rectangles[M];
				auto VertexID = FindVertex(RVertexes, RectangleMap.mappingForVariable(M));
				auto& Vertex = RVertexes[VertexID];
				auto& Node = Result["Nodes"][M];
				Node["ID"] = Vertex->id();
				auto X = Rectangle->getCentreX();
				auto Y = Rectangle->getCentreY();
				Node["X"] = X; Node["Y"] = Y;
			}
			Result["Statistics"]["NodesChanged"] = Rectangles.size();
		} catch (std::exception ex) {
			Result["Succeeded"] = false;
#ifndef NDEBUG
		} catch (vpsc::CriticalFailure ex) {
			Result["Succeeded"] = false;
#endif
		}
		// Output the result
		return ReturnJSON(Result, Output);
	}
	/*CalculateRoute: computes the wire routing of the given circuit.*/
	CIVITAS_EXPORT const int CalculateRoute(int ID, char* Input, char* Output) {
		Json Result; int Changes = ImportChanges(ID, Input);
		// No changes
		if (Changes == 0) return ReturnJSON(Result, Output);
		// Read the data
		auto& Router = Routers[ID];
		auto& RVertexes = *Shapes[ID];
		auto& REdges = *Connectors[ID];
		auto Modifies = ReadJSON(Input);
		// Layout support
		auto RectangleMap = cola::VariableIDMap();
		auto Rectangles = std::vector<vpsc::Rectangle*>();
		auto Cluster = cola::RootCluster();
		auto Constraints = cola::CompoundConstraints();
		// Layout helpers
		MapRectangles(RVertexes, RectangleMap, Rectangles, Cluster);
		// Remap
		/*for (int M = 0; M < Rectangles.size(); M++) {
			auto ID = RectangleMap.mappingForVariable(M, true);
			auto& Vertex = RVertexes[FindVertex(RVertexes, ID)];
			auto& Rectangle = Rectangles[M];
			auto ARectangle = new Avoid::Rectangle(
				Avoid::Point(Rectangle->getMinX(), Rectangle->getMinY()),
				Avoid::Point(Rectangle->getMaxX(), Rectangle->getMaxY()));
			Router->moveShape(Vertex, *ARectangle);
		}*/
		// Routing
		//auto Plugin = topology::AvoidTopologyAddon(Rectangles, Constraints, &Cluster, RectangleMap, 2000);
		//Router->setTopologyAddon(&Plugin);
		try {
			Router->processTransaction();
			Result["Succeeded"] = true;
			ExportChanges(Result, RVertexes, REdges);
		} catch (std::exception ex) {
			Result["Succeeded"] = false;
#ifndef NDEBUG
		} catch (vpsc::CriticalFailure ex) {
			Result["Succeeded"] = false;
#endif
		}
		// Output the result
		return ReturnJSON(Result, Output);
	}
}
