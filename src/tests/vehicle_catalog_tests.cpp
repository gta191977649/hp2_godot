#include "port/vehicle/catalog.hpp"
#include "port/vehicle/texture_state.hpp"
#include "port/vehicle/model.hpp"
#include "platform/ps2_asset_root.hpp"
#include <iostream>
#include <set>
#include <stdexcept>
#include <cmath>
#include <cstring>
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(int argc,char** argv) {
    const bool all_skins=argc==2 && std::strcmp(argv[1],"--all-skins")==0;
    check(argc==1||all_skins,"Expected optional --all-skins");
    const auto root=hp2::platform::resolve_zzdata_root(".");
    if(!root){
        const auto* configured=std::getenv(hp2::platform::kAssetRootEnvVar);
        check(!(configured&&*configured)&&!hp2::platform::resolve_local_config_value(".","root"),"Configured original asset root is invalid");
        std::cout<<"SKIPPED: original vehicle catalogs not configured\n";return 77;
    }
    port::assets::DirectorySource source(*root);
    port::vehicle::Catalog catalog(source);catalog.load();
    check(catalog.cars().size()==64,"Original car rows must all be retained");
    check(catalog.skins().size()==235,"Original skin rows must all be retained");
    check(catalog.car(22).code=="SPIDERHP" && catalog.car(22).prefix=="CHALLENGE","HP geometry prefix is original data");
    check(catalog.car(22).resource_path=="CARS/CHALLENGE/GEOMETRY.BIN","HP geometry path must use the original resource mapping");
    check(catalog.car(40).resource_path==catalog.car(39).resource_path,"Original shared resources must remain shared");
    check(catalog.skin(39,1).name=="Arctic Silver" && catalog.skin(39,1).material_hash==0x93c0eaf6,"Original skin name and material");
    std::size_t referenced=0;
    for(const auto& car:catalog.cars()) {
        check(!car.code.empty(),"Original car code is missing");
        referenced+=car.skins.size();
        check(car.available || !car.availability_error.empty(),"Missing resources require an explicit diagnostic");
    }
    check(referenced==catalog.skins().size(),"Skin membership must preserve all original rows");
    bool failed=false;try{catalog.skin(39,255);}catch(const std::out_of_range&){failed=true;}
    check(failed,"Unknown skin must not choose another color silently");
    if(all_skins){
        unsigned loaded=0,unavailable=0,failures=0;
        for(const auto& car:catalog.cars()){
            port::vehicle::Model model; // Reuse original geometry between this car's skins.
            for(const auto& skin:car.skins){
                if(!car.available){
                    ++unavailable;
                    std::cout<<"UNAVAILABLE car="<<car.id<<" skin="<<skin.id<<" "<<car.code<<" "<<car.availability_error<<std::endl;
                    continue;
                }
                try{
                    model.load_id(source,int(car.id),int(skin.id));
                    check(!model.parts().empty()&&!model.textures().empty(),"Original skin must bind parts and textures");
                    for(const auto& texture:model.textures()){
                        const auto state=port::vehicle::texture_state_00108CC0(texture.header);
                        (void)port::vehicle::test_state_00105AC0(texture.header);
                        check(!state.repeat_u&&!state.repeat_v,"Static viewer requires the confirmed clamp-both-axes state");
                        if(texture.header.alpha_bits!=0x44)continue;
                        check(texture.header.Field_0x74==0,"Static source-over texture unexpectedly enables flag4");
                        for(std::size_t i=3;i<texture.bitmap.rgba.size();i+=4)
                            check(texture.bitmap.rgba[i]<=127,"Static source-over raw GS alpha exceeds the confirmed corpus bound");
                    }
                    std::set<std::size_t> solids;
                    for(const auto& part:model.parts())if(solids.insert(part.solid).second){
                        check(part.solid<model.geometry().solids.size(),"Bound solid index outside original geometry");
                        for(const auto& surface:model.geometry().solids[part.solid].surfaces){
                            for(const auto& vertex:surface.vertices)
                                check(std::isfinite(vertex.uv[0])&&std::isfinite(vertex.uv[1]),"Nonfinite adjusted original UV");
                            if(!(surface.render_flags&0x400)&&!surface.indices.empty()){
                                check(model.materials().count(surface.material_hash)!=0,"Original surface material is missing");
                                bool texture_found=false;
                                for(const auto& texture:model.textures())if(texture.header.tex_hash==surface.texture_hash){
                                    texture_found=true;
                                    if(texture.header.alpha_bits==0x44){
                                        check((surface.render_flags&4)==0,"Static source-over strip unexpectedly enables flag4");
                                        check(part.lights[3]==0&&part.lights[7]==0&&part.lights[11]==0&&part.lights[15]==128,
                                              "Initialized frontend alpha coefficients changed");
                                    }
                                }
                                check(texture_found,"Original surface texture is missing");
                            }
                        }
                    }
                    ++loaded;
                    std::cout<<"LOADED car="<<car.id<<" skin="<<skin.id<<" "<<car.code<<" parts="<<model.parts().size()
                             <<" textures="<<model.textures().size()<<" materials="<<model.materials().size()<<std::endl;
                }catch(const std::exception& error){
                    ++failures;
                    std::cout<<"FAILED car="<<car.id<<" skin="<<skin.id<<" "<<car.code<<" "<<error.what()<<std::endl;
                }
            }
        }
        std::cout<<"VEHICLE_ALL_SKINS loaded="<<loaded<<" unavailable="<<unavailable<<" failed="<<failures<<std::endl;
        check(loaded+unavailable+failures==catalog.skins().size(),"Every original skin association must be checked");
        check(failures==0,"Original available skin load failed");
    }
    std::cout<<"VEHICLE_CATALOG_OK cars=64 skins=235\n";
}
