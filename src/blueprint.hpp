#pragma once
#include <cstdint>
#include <array>
#include <map>
#include <sstream>
#include <string>
#include <span>
#include <vector>
#include <cstring>
#include "trajectory.hpp"

namespace fc::blueprint {
// Indexes refer to the serialized building list, not world-specific building IDs.
struct Metadata {
    int buildings{};
    std::map<int, Limits> fountains;
};
inline bool valid(Limits v) {
    return std::isfinite(v.drop) && v.drop>=0 && v.drop<=1000 && std::fmod(v.drop,50)==0
        && (v.horizontal==-1 || (std::isfinite(v.horizontal) && v.horizontal>=0
            && v.horizontal<=500 && std::fmod(v.horizontal,50)==0));
}
inline std::string encode(const Metadata& data, const std::string& hash) {
    std::ostringstream out;
    out << "FountainControlBlueprint 1\n" << hash << '\n' << data.buildings << ' ' << data.fountains.size() << '\n';
    for (auto [index,value]:data.fountains)
        out << index << ' ' << value.drop << ' ' << value.horizontal << '\n';
    return out.str();
}
inline std::optional<Metadata> decode(const std::string& text,const std::string& hash,int buildings) {
    if (text.size()>1048576 || hash.size()!=64 || buildings<0 || buildings>100000) return {};
    std::istringstream in(text);
    std::string magic, fingerprint;
    int version, count;
    Metadata result;
    if (!(in>>magic>>version>>fingerprint>>result.buildings>>count) || magic!="FountainControlBlueprint"
        || version!=1 || fingerprint!=hash || result.buildings!=buildings || count<0 || count>buildings) return {};
    for (int n=0;n<count;++n) {
        int index; Limits value;
        if (!(in>>index>>value.drop>>value.horizontal) || index<0 || index>=buildings || !valid(value)
            || !result.fountains.emplace(index,value).second) return {};
    }
    in>>std::ws;
    if (!in.eof()) return {};
    return result;
}
constexpr std::array<uint8_t,8> png_signature{137,80,78,71,13,10,26,10};
constexpr char chunk_type[]="fcRs"; // Ancillary, private, reserved bit clear, safe to copy.
inline uint32_t crc32(std::span<const uint8_t> bytes) {
    uint32_t crc=0xffffffff;
    for (auto b:bytes) {
        crc^=b;
        for (int bit=0;bit<8;++bit) crc=(crc>>1)^(0xedb88320u & (0u-(crc&1)));
    }
    return ~crc;
}
inline uint32_t big32(const uint8_t* p) { return uint32_t(p[0])<<24 | uint32_t(p[1])<<16 | uint32_t(p[2])<<8 | p[3]; }
inline void append32(std::vector<uint8_t>& bytes,uint32_t n) {
    for (int shift=24;shift>=0;shift-=8) bytes.push_back(static_cast<uint8_t>(n>>shift));
}
struct Png {
    std::vector<uint8_t> canonical; // The complete PNG and native tail, excluding only fcRs chunks.
    size_t iend{};
    std::optional<std::string> metadata;
};
inline std::optional<Png> extract(std::span<const uint8_t> bytes) {
    if (bytes.size()<8 || !std::equal(png_signature.begin(),png_signature.end(),bytes.begin())) return {};
    Png result;
    result.canonical.insert(result.canonical.end(),bytes.begin(),bytes.begin()+8);
    size_t cursor=8;
    int metadata_chunks=0;
    while (cursor<=bytes.size() && bytes.size()-cursor>=12) {
        auto length=big32(bytes.data()+cursor);
        if (length>bytes.size()-cursor-12) return {};
        auto chunk=bytes.subspan(cursor,static_cast<size_t>(length)+12);
        bool ours=std::memcmp(chunk.data()+4,chunk_type,4)==0;
        if (ours) {
            ++metadata_chunks;
            if (metadata_chunks==1 && length<=1048576 && crc32(chunk.subspan(4,length+4))==big32(chunk.data()+length+8))
                result.metadata=std::string(reinterpret_cast<const char*>(chunk.data()+8),length);
            else result.metadata.reset();
        } else {
            if (std::memcmp(chunk.data()+4,"IEND",4)==0) {
                if (length!=0) return {};
                result.iend=result.canonical.size();
                result.canonical.insert(result.canonical.end(),bytes.begin()+cursor,bytes.end());
                return result;
            }
            result.canonical.insert(result.canonical.end(),chunk.begin(),chunk.end());
        }
        cursor+=chunk.size();
    }
    return {};
}
inline std::vector<uint8_t> embed(const Png& png,const std::string& metadata) {
    if (metadata.size()>1048576) return {};
    std::vector<uint8_t> result;
    result.reserve(png.canonical.size()+metadata.size()+12);
    result.insert(result.end(),png.canonical.begin(),png.canonical.begin()+png.iend);
    append32(result,static_cast<uint32_t>(metadata.size()));
    auto start=result.size();
    result.insert(result.end(),chunk_type,chunk_type+4);
    result.insert(result.end(),metadata.begin(),metadata.end());
    append32(result,crc32(std::span(result).subspan(start)));
    result.insert(result.end(),png.canonical.begin()+png.iend,png.canonical.end());
    return result;
}
}
