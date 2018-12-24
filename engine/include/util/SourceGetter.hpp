#ifndef HEADER_UTIL_SOURCEGETTER_H
#define HEADER_UTIL_SOURCEGETTER_H

//#include "SourceParser.hpp"
//#include "SourceDescription.hpp"

template <typename S>
class SourceGetter{
	public:
		virtual S getSource() = 0;
		virtual ~SourceGetter(){};
};

/*template <typename S> 
class StaticSourceGetter : public SourceGetter<S>{
	public:
		S source;
		
		StaticSourceGetter(SourceDiscription& sourceDes,SourceParser<S>& parser){
			source = parser.parse(sourceDes.open());
		}
		
		S getSource(){
			return source;
		}
};

template <typename S>
class DynamicSourceGetter : public SourceGetter<S>{
	public:
		SourceParser<S> parser;
		SourceDiscription sourceDes;
		
		DynamicSourceGetter(SourceDiscription& sourceDes,SourceParser<S>& parser){
			this->parser = parser;
			this->sourceDes = sourceDes;
		}
		
		S getSource(){
			return parser.parse(sourceDes.open);
		}
};*/

template <typename S>
class DirectSourceGetter : public SourceGetter<S>{
	public:
		S source;
		
		DirectSourceGetter(S source){
			this->source = source;
		}
		
		S getSource(){
			return source;
		}
};

template <typename S>
class NullSourceGetter : public SourceGetter<S>{
	public:
		S getSource(){
			return NULL;
		}
};

#endif
