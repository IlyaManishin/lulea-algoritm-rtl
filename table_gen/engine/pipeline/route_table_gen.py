from pathlib import Path

from ..core.mrt_gen import generate_mrt_routes
from ..core.gen_types import MrtRecord, PortMapping, RouteRecord
from ..core.mrt_reader import MRTFileType
from ..core.random_gen import generate_random_routes

from ..config import GeneratorType, ROUTE_COUNT, PORT_MAP_EXTENSION
from ..utils.port_mapping import map_next_hops_to_ports
from ..utils.route_writer import save_routes_and_ports


def generate_route_table(
    generator_type: GeneratorType,
    output_path: str | Path,
    input_path: str | Path | None = None,
    count: int = ROUTE_COUNT,
    seed: int | None = 42,
    mrt_file_type: MRTFileType = MRTFileType.TEXT,
) -> None:
    raw_routes: list[MrtRecord]
    if generator_type == GeneratorType.MRT:
        if input_path is None:
            raise ValueError(
                "input_path is required when generator_type is GeneratorType.MRT")
        raw_routes = generate_mrt_routes(
            input_path=input_path,
            count=count,
            file_type=mrt_file_type,
            seed=seed,
        )
    elif generator_type == GeneratorType.RANDOM:
        raw_routes = generate_random_routes(
            count=count,
            seed=seed,
        )
    else:
        raise ValueError(f"Unsupported generator type: {generator_type}")

    port_map: PortMapping = map_next_hops_to_ports(raw_routes)
    hop_to_port: dict[str, int] = {hop: pid for pid, hop in port_map.items()}
    routes: list[RouteRecord] = [
        RouteRecord(prefix=r.prefix, mask=r.mask, port_id=hop_to_port[r.next_hop])
        for r in raw_routes
    ]
    port_map_path = Path(output_path).with_suffix(PORT_MAP_EXTENSION)

    save_routes_and_ports(
        routes=routes,
        port_map=port_map,
        routes_path=output_path,
        port_map_path=port_map_path,
    )
